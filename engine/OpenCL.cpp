#include "config.h"
#ifdef USE_OPENCL

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <array>
#include <thread>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "Utils.h"
#include "Timing.h"
#include "OpenCL.h"
#include "Network.h"
#include "GTP.h"

using namespace Utils;

static std::string sourceCode_convolve15 = R"(
    __kernel
    __attribute__((work_group_size_hint(8, 16, 1)))
    void convolve5(
                   __global const float * in,
                   __global float * merge,
                   __global const float * weights,
                   __local float * channel_buff,
                   __local float * row_buff) {

        // cl::NDRange global(channels, outputs, row);
        const int c   = get_global_id(0);  // channel
        const int o   = get_global_id(1);  // output
        const int row = get_global_id(2);  // row

        const int channels = get_global_size(0);
        const int outputs  = get_global_size(1);

        // cl::NDRange local(2, (1->32), 1);
        const int lx = get_local_id(0);
        const int ly = get_local_id(1);

        const int chan_buff_size = 8;
        const int out_buff_size  = get_local_size(1);
        const int row_buff_size  = 7;
        const int chan_shift     = 3;

        const int filter_size = 5;
        const int filter_len = filter_size * filter_size;
        const int mid = (filter_size / 2) + 1;
        const int extent = mid - 1;

        // input = channels * height * width
        // output = outputs * height * width
        // weights = output * channels * filter
        // merge = channels * outputs * height * width

        const int width = 19;
        const int height = 19;
        const int strip_size = filter_size * width;

        // Copy the input channels (strips) locally
        if (out_buff_size < 19 && ly == 0) {
            // strip-row
            for (int srow = 0; srow < filter_size; srow++) {
                int in_row = row - extent + srow;
                if ((unsigned)in_row >= height) {
                    for (int w = 0; w < width; w++) {
                        channel_buff[(lx * filter_size + srow) * width + w] = 0.0f;
                    }
                } else {
                    for (int w = 0; w < width; w++) {
                        channel_buff[(lx * filter_size + srow) * width + w] =
                            in[(c * height + in_row) * width + w];
                    }
                }
            }
        } else if (out_buff_size >= 19 && ly < 19) {
            // Every thread copies a column
            for (int srow = 0; srow < filter_size; srow++) {
                int in_row = row - extent + srow;
                float val = 0.0f;
                if ((unsigned)in_row < height) {
                    val = in[(c * height + in_row) * width + ly];
                }
                channel_buff[(lx * filter_size + srow) * width + ly] = val;
            }
        }

        __private float filter_buff[25];

        // Copy the filter we are applying locally
        // output * channel * filter_len
        for (int f = 0; f < filter_len; f++) {
            filter_buff[f] = weights[(o * channels + c) * filter_len + f];
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        int out_lane = 0;
        int out_cw   = 0;
        #pragma unroll
        for (int cw = 0; cw < width; cw++) {
            int fwstart = cw - extent;
            int fwend   = cw + extent;
            float out;
            // Start filter
            if (fwstart >= 0 && fwend < width) {
                int fid = lx * strip_size + fwstart;
                out  = channel_buff[fid              ] * filter_buff[0];
                out += channel_buff[fid           + 1] * filter_buff[1];
                out += channel_buff[fid           + 2] * filter_buff[2];
                out += channel_buff[fid           + 3] * filter_buff[3];
                out += channel_buff[fid           + 4] * filter_buff[4];

                out += channel_buff[fid + width      ] * filter_buff[5];
                out += channel_buff[fid + width   + 1] * filter_buff[6];
                out += channel_buff[fid + width   + 2] * filter_buff[7];
                out += channel_buff[fid + width   + 3] * filter_buff[8];
                out += channel_buff[fid + width   + 4] * filter_buff[9];

                out += channel_buff[fid + width*2    ] * filter_buff[10];
                out += channel_buff[fid + width*2 + 1] * filter_buff[11];
                out += channel_buff[fid + width*2 + 2] * filter_buff[12];
                out += channel_buff[fid + width*2 + 3] * filter_buff[13];
                out += channel_buff[fid + width*2 + 4] * filter_buff[14];

                out += channel_buff[fid + width*3    ] * filter_buff[15];
                out += channel_buff[fid + width*3 + 1] * filter_buff[16];
                out += channel_buff[fid + width*3 + 2] * filter_buff[17];
                out += channel_buff[fid + width*3 + 3] * filter_buff[18];
                out += channel_buff[fid + width*3 + 4] * filter_buff[19];

                out += channel_buff[fid + width*4    ] * filter_buff[20];
                out += channel_buff[fid + width*4 + 1] * filter_buff[21];
                out += channel_buff[fid + width*4 + 2] * filter_buff[22];
                out += channel_buff[fid + width*4 + 3] * filter_buff[23];
                out += channel_buff[fid + width*4 + 4] * filter_buff[24];
            } else {
                const float * filter_idx = filter_buff;
                out = 0.0f;
                #pragma unroll
                for (int fh = 0; fh < filter_size; fh++) {
                    for (int fw = fwstart; fw <= fwend; fw++) {
                        // "zero padding"
                        if ((unsigned)fw >= width) {
                            filter_idx++;
                            continue;
                        }

                        float input = channel_buff[(lx * filter_size + fh) * width + fw];
                        out += input * *filter_idx++;
                    }
                }
            }
            // End filter
            row_buff[(ly * chan_buff_size + lx) * row_buff_size + out_lane] = out;
            out_lane++;

            // Row buffer full or last lane?
            if (out_lane == row_buff_size || (cw == width - 1)) {
                barrier(CLK_LOCAL_MEM_FENCE);
                if (lx < out_lane) {
                    float val;
                    val  = row_buff[(ly * chan_buff_size + 0) * row_buff_size + lx];
                    val += row_buff[(ly * chan_buff_size + 1) * row_buff_size + lx];
                    val += row_buff[(ly * chan_buff_size + 2) * row_buff_size + lx];
                    val += row_buff[(ly * chan_buff_size + 3) * row_buff_size + lx];
                    val += row_buff[(ly * chan_buff_size + 4) * row_buff_size + lx];
                    val += row_buff[(ly * chan_buff_size + 5) * row_buff_size + lx];
                    val += row_buff[(ly * chan_buff_size + 6) * row_buff_size + lx];
                    val += row_buff[(ly * chan_buff_size + 7) * row_buff_size + lx];
                    merge[(((c >> chan_shift) * height + row) * width + out_cw + lx) * outputs + o] = val;
                }
                out_cw  += row_buff_size;
                out_lane = 0;
            }
        }
    }
)";

static std::string sourceCode_convolve3 = R"(
    __kernel
    __attribute__((work_group_size_hint(8, 32, 1)))
    void convolve3(
                   __global const float * in,
                   __global float * merge,
                   __global const float * weights,
                   __local float * channel_buff,
                   __local float * row_buff,
                   const int row_tile_size) {

        // cl::NDRange global(channels, outputs, row);
        const int c   = get_global_id(0);  // channel
        const int o   = get_global_id(1);  // output
        const int r   = get_global_id(2);  // row

        const int channels = get_global_size(0);
        const int outputs  = get_global_size(1);

        // cl::NDRange local(2, (1->32), 1);
        const int lx = get_local_id(0);
        const int ly = get_local_id(1);

        const int chan_buff_size = 8;
        const int out_buff_size  = get_local_size(1);
        const int row_buff_size  = 7;
        const int chan_shift     = 3;

        const int width = 19;
        const int height = 19;

        const int filter_size = 3;
        const int filter_len = filter_size * filter_size;
        const int mid = (filter_size / 2) + 1;
        const int extent = mid - 1;
        const int pad_width = width + filter_size - 1;

        // input = channels * height * width
        // output = outputs * height * width
        // weights = output * channels * filter
        // merge = channels * outputs * height * width

        __private float filter_buff[9];
        __private float chan_cache[2];
        __private float stripe_cache[9];

        // Copy the filter we are applying locally
        // output * channel * filter_len
        for (int f = 0; f < filter_len; f++) {
            filter_buff[f] = weights[(o * channels + c) * filter_len + f];
        }

        for (int tile = 0; tile < row_tile_size; tile++) {
            int row = r * row_tile_size + tile;
            if (row > 18) break;

            // Copy the input channels (strips) locally
            if (out_buff_size < 21 && ly == 0) {
                // strip-row
                for (int srow = 0; srow < filter_size; srow++) {
                    int in_row = row - extent + srow;
                    channel_buff[(lx * pad_width + 0) * filter_size + srow]             = 0.0f;
                    if ((unsigned)in_row < height) {
                        for (int w = 0; w < width; w++) {
                            float val = in[(c * height + in_row) * width + w];
                            channel_buff[(lx * pad_width + w + extent) * filter_size + srow] = val;
                        }
                    } else {
                        for (int w = 0; w < width; w++) {
                            channel_buff[(lx * pad_width + w + extent) * filter_size + srow] = 0.0f;
                        }
                    }
                    channel_buff[(lx * pad_width + pad_width - 1) * filter_size + srow] = 0.0f;
                }
            } else if (out_buff_size >= 21 && ly < 21) {
                // Every thread copies a column
                int copy_idx = (lx * pad_width + ly) * filter_size;
                if (tile == 0 || row == 18) {
                    // Every thread copies a column
                    for (int srow = 0; srow < filter_size; srow++) {
                        int in_row = row - extent + srow;
                        float val = 0.0f;
                        if ((unsigned)in_row < height && ly >= 1 && ly <= 19) {
                            val = in[(c * height + in_row) * width + ly - 1];
                        }
                        channel_buff[copy_idx + srow] = val;
                        if (srow > 0) {
                            chan_cache[srow - 1] = val;
                        }
                    }
                } else {
                    int in_row = row - extent + 2;
                    float val = 0.0f;
                    if (ly >= 1 && ly <= 19) {
                        val = in[(c * height + in_row) * width + ly - 1];
                    }
                    channel_buff[copy_idx + 0] = chan_cache[0];
                    channel_buff[copy_idx + 1] = chan_cache[1];
                    channel_buff[copy_idx + 2] = val;
                    chan_cache[0] = chan_cache[1];
                    chan_cache[1] = val;
                }
            }

            int out_lane = 0;
            int out_cw   = 0;
            __local float * out_row_buff = &row_buff[(ly * chan_buff_size + lx) * row_buff_size];
            int fid = (lx * pad_width) * filter_size;
            barrier(CLK_LOCAL_MEM_FENCE);

            for (int rc = 0; rc < 9; rc++) {
                stripe_cache[rc] = channel_buff[fid + rc];
            }

            #pragma unroll
            for (int cw = 0; cw < width; cw++) {
                // Start filter
                float out  =   stripe_cache[      0] * filter_buff[0]
                             + stripe_cache[      1] * filter_buff[3]
                             + stripe_cache[      2] * filter_buff[6]
                             + stripe_cache[      3] * filter_buff[1]
                             + stripe_cache[      4] * filter_buff[4]
                             + stripe_cache[      5] * filter_buff[7]
                             + stripe_cache[      6] * filter_buff[2]
                             + stripe_cache[      7] * filter_buff[5]
                             + stripe_cache[      8] * filter_buff[8];
                // End filter
                out_row_buff[out_lane++] = out;
                fid += filter_size;

                for (int rc = 0; rc < 6; rc++) {
                    stripe_cache[rc] = stripe_cache[rc + 3];
                }
                stripe_cache[6] = channel_buff[fid + 6];
                stripe_cache[7] = channel_buff[fid + 7];
                stripe_cache[8] = channel_buff[fid + 8];

                // Row buffer full or last lane?
                if (out_lane == row_buff_size || (cw == width - 1)) {
                    barrier(CLK_LOCAL_MEM_FENCE);
                    if (lx < out_lane) {
                        // lx = channels 2 or 8, ly = outputs 32
                        // repurpose the lx threads over columns now
                        float val;
                        val  = row_buff[(ly * chan_buff_size + 0) * row_buff_size + lx];
                        val += row_buff[(ly * chan_buff_size + 1) * row_buff_size + lx];
                        val += row_buff[(ly * chan_buff_size + 2) * row_buff_size + lx];
                        val += row_buff[(ly * chan_buff_size + 3) * row_buff_size + lx];
                        val += row_buff[(ly * chan_buff_size + 4) * row_buff_size + lx];
                        val += row_buff[(ly * chan_buff_size + 5) * row_buff_size + lx];
                        val += row_buff[(ly * chan_buff_size + 6) * row_buff_size + lx];
                        val += row_buff[(ly * chan_buff_size + 7) * row_buff_size + lx];
                        merge[(((c >> chan_shift) * height + row) * width + out_cw + lx) * outputs + o] = val;
                    }
                    out_cw  += row_buff_size;
                    out_lane = 0;
                }
            }
        }
    }
)";

static std::string sourceCode_utility = R"(
    __kernel void merge(
                        __global const float * in,
                        __global float * out,
                        __constant const float * biases,
                        __private const int channels) {

        // cl::NDRange global(outputs, 19*19);
        const int gx = get_global_id(0);
        const int gy = get_global_id(1);

        const int output = gx;
        const int b = gy;
        const int outputs = get_global_size(0);

        const int width = 19;
        const int height = 19;
        const int boardsize = width * height;

        const int o = output;
        const float bias = biases[o];

        float sum = bias;
        for (int c = 0; c < channels; c++) {
            sum += in[(c * boardsize + b) * outputs + o];
        }
        // ELU
        sum = sum > 0 ? sum : 1.0f * (half_exp(sum) - 1.0f);
        out[o * boardsize + b] = sum;
    }

    __kernel void batchnorm(
                        __global const float * in,
                        __global float * out,
                        __constant const float * means,
                        __constant const float * variances,
                        __constant const float * scale) {

        // cl::NDRange global(outputs, 19*19);
        const int gx = get_global_id(0);
        const int gy = get_global_id(1);

        const int output = gx;
        const int outputs      = get_global_size(0);
        const int channel_size = get_global_size(1);

        const unsigned int o = output;
        const unsigned int b = gy;

        const float epsilon = 1e-5;

        const float mean = means[o] / scale[0];
        const float variance = epsilon + variances[o] / scale[0];
        const float scale_stddiv = 1.0f / sqrt(variance);

        out[o * channel_size + b] = scale_stddiv
                                    * (in[o * channel_size + b] - mean);
    }

    __kernel void innerproduct(
                               __private const int inputs,
                               __global const float * in,
                               __global float * out,
                               __global const float * weights,
                               __constant const float * biases) {
        const int gx = get_global_id(0);
        const int output = gx;

        const int outputs = get_global_size(0);

        const unsigned int o = output;
        unsigned int i;

        float16 val16 = (float16)(0.0f);
        for (i = 0; i + 16 < inputs; i += 16) {
            val16 += vload16(0, &in[i]) * vload16(0, &weights[o * inputs + i]);
        }
        float val = val16.s0 + val16.s1 + val16.s2 + val16.s3
                  + val16.s4 + val16.s5 + val16.s6 + val16.s7
                  + val16.s8 + val16.s9 + val16.sa + val16.sb
                  + val16.sc + val16.sd + val16.se + val16.sf;

        for (; i < inputs; i++) {
            val += in[i] * weights[o * inputs + i];
        }
        val += biases[o];
        if (outputs > 1) {
            val = val > 0 ? val : 1.0f * (half_exp(val) - 1.0f);
        }
        out[o] = val;
    }
)";

OpenCL opencl;
OpenCL_Network opencl_policy_net;
OpenCL_Network opencl_value_net;
thread_local ThreadData opencl_thread_data;

bool OpenCL::thread_can_issue() {
    static std::atomic<int> max_queue_size{0};
    int current_queue = opencl_thread_data.m_results_outstanding;
    if (current_queue > max_queue_size) {
        max_queue_size = current_queue;
        //myprintf("qsz: %d\n", max_queue_size);
    }
    return current_queue < 2;
}

std::atomic<int> * OpenCL::get_thread_results_outstanding() {
    return &opencl_thread_data.m_results_outstanding;
}

void OpenCL::ensure_thread_initialized() {
    if (!opencl_thread_data.m_is_initialized) {
        // Make kernels
        opencl_thread_data.m_convolve3_kernel = cl::Kernel(m_program, "convolve3");
        opencl_thread_data.m_convolve5_kernel = cl::Kernel(m_program, "convolve5");
        opencl_thread_data.m_merge_kernel = cl::Kernel(m_program, "merge");
        opencl_thread_data.m_batchnorm_kernel = cl::Kernel(m_program, "batchnorm");
        opencl_thread_data.m_innerproduct_kernel = cl::Kernel(m_program, "innerproduct");
        opencl_thread_data.m_commandqueue = cl::CommandQueue(cl::Context::getDefault(),
                                                             cl::Device::getDefault());
        opencl_thread_data.m_is_initialized = true;
    }
}

void OpenCL_Network::add_weights(size_t layer,
                                 size_t size,
                                 const float * weights) {
    if (layer >= m_layers.size()) {
        m_layers.push_back(Layer());
    }

    size_t weightSize = size *
        sizeof(std::remove_pointer<decltype(weights)>::type);

    cl::Buffer bufferWeights = cl::Buffer(CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                          weightSize, const_cast<float*>(weights));

    m_layers.back().weights.push_back(bufferWeights);
}

void OpenCL_Network::forward(std::vector<float>& input,
                             std::vector<float>& output,
                             event_callback cb, void * data) {
    constexpr int width = 19;
    constexpr int height = 19;
    constexpr size_t one_plane = width * height * sizeof(float);

    opencl.ensure_thread_initialized();
    opencl_thread_data.m_results_outstanding.fetch_add(1, std::memory_order_release);
    size_t inSize = sizeof(float) * input.size();
    size_t outSize = sizeof(float) * output.size();
    size_t finalSize = m_layers.back().outputs * 19 * 19 * sizeof(float);

    if (!opencl_thread_data.m_buffers_allocated) {
        size_t alloc_inSize = one_plane * Network::MAX_CHANNELS;
        size_t alloc_outSize = one_plane * Network::MAX_CHANNELS;
        size_t alloc_finalSize = one_plane * Network::MAX_CHANNELS;
        size_t alloc_mergeSize = one_plane *
            Network::MAX_CHANNELS * (Network::MAX_CHANNELS / 8);

        opencl_thread_data.m_inBuffer = cl::Buffer(
            CL_MEM_READ_WRITE, alloc_inSize);
        opencl_thread_data.m_tmpBuffer = cl::Buffer(
            CL_MEM_READ_WRITE, alloc_outSize);
        opencl_thread_data.m_mergeBuffer = cl::Buffer(
            CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, alloc_mergeSize);
        opencl_thread_data.m_outBuffer = cl::Buffer(
            CL_MEM_WRITE_ONLY, alloc_finalSize);
        opencl_thread_data.m_buffers_allocated = true;
    }

    cl::Buffer & inBuffer = opencl_thread_data.m_inBuffer;
    cl::Buffer & outBuffer = opencl_thread_data.m_outBuffer;
    cl::Buffer & tmpBuffer = opencl_thread_data.m_tmpBuffer;
    cl::Buffer & mergeBuffer = opencl_thread_data.m_mergeBuffer;
    cl::CommandQueue & queue = opencl_thread_data.m_commandqueue;

    queue.enqueueWriteBuffer(inBuffer, CL_FALSE, 0, inSize, input.data());

    for (auto & layer : m_layers) {
        if (layer.is_batchnorm) {
            batchnorm(layer.outputs,
                      layer.filter_size,
                      inBuffer,
                      tmpBuffer,
                      layer.weights);
            std::swap(inBuffer, tmpBuffer);
        } else if (layer.is_innerproduct) {
            innerproduct(layer.channels,
                         layer.outputs,
                         inBuffer,
                         tmpBuffer,
                         layer.weights);
            std::swap(inBuffer, tmpBuffer);
        } else {
            // convolution
            convolve(layer.filter_size,
                     layer.channels,
                     layer.outputs,
                     inBuffer,
                     tmpBuffer,
                     mergeBuffer,
                     layer.weights);
            std::swap(inBuffer, tmpBuffer);
        }
    }

    queue.enqueueCopyBuffer(inBuffer, outBuffer, 0, 0, finalSize);
    queue.enqueueReadBuffer(outBuffer, CL_FALSE, 0, finalSize, output.data());

    opencl.m_cb_outstanding.fetch_add(1, std::memory_order_release);
    queue.finish();
    if (cb != nullptr) {
        cb(CL_COMPLETE, 0, data);
    } else {
        assert(data == nullptr);
        opencl_thread_data.m_results_outstanding.fetch_sub(1, std::memory_order_release);
        opencl.callback_finished();
    }
}

void OpenCL::callback_finished() {
    m_cb_outstanding.fetch_sub(1, std::memory_order_release);
}

void OpenCL::join_outstanding_cb() {
    while (m_cb_outstanding.load(std::memory_order_acquire) > 0);
}

void OpenCL_Network::convolve(int filter_size, int channels, int outputs,
                              cl::Buffer& bufferInput,
                              cl::Buffer& bufferOutput,
                              cl::Buffer& bufferMerge,
                              std::vector<cl::Buffer>& weights) {
    // fixed for 19x19
    constexpr int width = 19;
    constexpr int height = 19;
    constexpr int boardsize = width * height;
    unsigned int filter_len = filter_size * filter_size;

    size_t inSize = width * height * channels * sizeof(float);

    // Every input channel is this big
    size_t chanSize = width * height * sizeof(float);

    size_t outputGroup;

    cl::Kernel * m_convolve_kernel = nullptr;
    if (filter_size == 3) {
        m_convolve_kernel = &opencl_thread_data.m_convolve3_kernel;
    } else {
        assert(filter_size == 5);
        m_convolve_kernel = &opencl_thread_data.m_convolve5_kernel;
    }

    constexpr int channelGroup = 8;
    constexpr int channelShift = 3;
    constexpr int rowGroup = 1;
    // Workgroup things
    if (opencl.m_max_workgroup_size < 512
        || opencl.m_max_workgroup_dims[1] < 64) {
        outputGroup = std::min(outputs, 32);
    } else {
        // Can optionally be 64
        outputGroup = std::min(outputs, 32);
    }

    if (outputs == 48) {
        outputGroup = 16;
    }

    // Total output size after reducing
    size_t outSize = width * height * outputs * sizeof(float);

    // Produce channel * output planes and merge them at the end
    size_t mergeSize = (channels >> channelShift) * outSize;

    // Store the filters locally
    // size_t filtSize = outputGroup * channelGroup * filter_len * sizeof(float);

    // Copy the rows locally
    size_t stripSize;
    int rowTileSize;
    int rowTiles;
    if (filter_size == 3) {
        stripSize = filter_size * (width + (filter_size - 1)) * sizeof(float);
        rowTiles    =  cfg_rowtiles;
        rowTileSize =  (19 + rowTiles - 1) / rowTiles;
    } else if (filter_size == 5) {
        stripSize = filter_size * width * sizeof(float);
        rowTiles    = 19;
        rowTileSize =  1;
    }

    int rowBuffer = std::min<int>(channelGroup, 7);
    assert(rowBuffer == 7); // hardcoded in kernel
    size_t rowSize = channelGroup * outputGroup * rowBuffer * sizeof(float);

    assert(mergeSize <= bufferMerge.getInfo<CL_MEM_SIZE>());

    cl::CommandQueue & queue = opencl_thread_data.m_commandqueue;

    try {
        m_convolve_kernel->setArg(0, bufferInput);
        m_convolve_kernel->setArg(1, bufferMerge);
        m_convolve_kernel->setArg(2, weights[0]);
        m_convolve_kernel->setArg(3, cl::Local(stripSize * channelGroup * rowGroup));
        m_convolve_kernel->setArg(4, cl::Local(rowSize));
        if (filter_size == 3) {
            m_convolve_kernel->setArg(5, rowTileSize);
        }

        queue.enqueueNDRangeKernel(*m_convolve_kernel, cl::NullRange,
                                   cl::NDRange(channels, outputs, rowTiles),
                                   cl::NDRange(channelGroup, outputGroup, rowGroup));
    } catch (const cl::Error &e) {
        std::cerr << "Error in convolve: " << e.what() << ": "
	        << e.err() << std::endl;
        throw;
    }

    cl::Kernel & merge_kernel = opencl_thread_data.m_merge_kernel;

    try {
        merge_kernel.setArg(0, bufferMerge);
        merge_kernel.setArg(1, bufferOutput);
        merge_kernel.setArg(2, weights[1]);
        merge_kernel.setArg(3, channels >> channelShift);

        queue.enqueueNDRangeKernel(merge_kernel, cl::NullRange,
                                   cl::NDRange(outputs, boardsize),
                                   cl::NDRange(std::min(8, outputs), 19));
    } catch (const cl::Error &e) {
        std::cerr << "Error in merge: " << e.what() << ": "
	        << e.err() << std::endl;
        throw;
    }
}

void OpenCL_Network::batchnorm(int outputs,
                               int channel_size,
                               cl::Buffer & bufferInput,
                               cl::Buffer & bufferOutput,
                               std::vector<cl::Buffer>& weights) {
    cl::CommandQueue & queue = opencl_thread_data.m_commandqueue;

    cl::Kernel & batchnorm_kernel = opencl_thread_data.m_batchnorm_kernel;

    size_t channelGroup = 1;
    if (channel_size == 361) {
        channelGroup = 19;
    }

    try {
        batchnorm_kernel.setArg(0, bufferInput);
        batchnorm_kernel.setArg(1, bufferOutput);
        batchnorm_kernel.setArg(2, weights[0]);
        batchnorm_kernel.setArg(3, weights[1]);
        batchnorm_kernel.setArg(4, weights[2]);

        queue.enqueueNDRangeKernel(batchnorm_kernel, cl::NullRange,
                                   cl::NDRange(outputs, channel_size),
                                   cl::NDRange(std::min(8, outputs), channelGroup));
    } catch (const cl::Error &e) {
        std::cerr << "Error in batchnorm: " << e.what() << ": "
            << e.err() << std::endl;
        throw;
    }
}

void OpenCL_Network::innerproduct(int inputs,
                                  int outputs,
                                  cl::Buffer & bufferInput,
                                  cl::Buffer & bufferOutput,
                                  std::vector<cl::Buffer>& weights) {
    cl::CommandQueue & queue = opencl_thread_data.m_commandqueue;

    cl::Kernel & innerproduct_kernel = opencl_thread_data.m_innerproduct_kernel;

    try {
        innerproduct_kernel.setArg(0, inputs);
        innerproduct_kernel.setArg(1, bufferInput);
        innerproduct_kernel.setArg(2, bufferOutput);
        innerproduct_kernel.setArg(3, weights[0]);
        innerproduct_kernel.setArg(4, weights[1]);

        queue.enqueueNDRangeKernel(innerproduct_kernel, cl::NullRange,
                                   cl::NDRange(outputs),
                                   cl::NDRange(std::min(16, outputs)));
    } catch (const cl::Error &e) {
        std::cerr << "Error in innerproduct: " << e.what() << ": "
            << e.err() << std::endl;
        throw;
    }
}

template<class T>
static std::string opencl_dev_type_to_string(T type) {
    if (type == CL_DEVICE_TYPE_CPU) {
        return "CPU";
    } else if (type == CL_DEVICE_TYPE_GPU) {
        return "GPU";
    } else if (type == CL_DEVICE_TYPE_ACCELERATOR) {
        return "Accelerator";
    } else {
        return "Unknown";
    }
}

static std::string trim(std::string trim_me) {
    boost::algorithm::trim(trim_me);
    return trim_me;
}

void OpenCL::initialize(void) {
    std::vector<cl::Platform> platforms;
    try {
        cl::Platform::get(&platforms);
    } catch (const cl::Error &e) {
        myprintf("OpenCL: %s\n", e.what());
        throw;
    }

    float best_version = 0.0f;
    cl::Platform best_platform;
    cl::Device best_device;
    std::string best_vendor;
    int best_score = 0;
    bool found_device = false;
    int id = 0;

    myprintf("Detected %d OpenCL platforms\n", platforms.size());

    for (auto &p : platforms) {
        std::string platvers = p.getInfo<CL_PLATFORM_VERSION>();
        std::string platprof = p.getInfo<CL_PLATFORM_PROFILE>();
        std::string platname = p.getInfo<CL_PLATFORM_NAME>();
        std::string platvend = p.getInfo<CL_PLATFORM_VENDOR>();
        myprintf("Platform version: %s\n", platvers.c_str());;
        myprintf("Platform profile: %s\n", platprof.c_str());
        myprintf("Platform name:    %s\n", platname.c_str());
        myprintf("Platform vendor:  %s\n", platvend.c_str());

        std::istringstream versstream(platvers);
        std::string tmp;
        float opencl_version;
        versstream >> tmp >> opencl_version;

        std::vector<cl::Device> devices;
        try {
            p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
        } catch (const cl::Error &e) {
            myprintf("Error getting device(s): %s: %d\n", e.what(), e.err());
            devices.clear();
        }
        for (auto& d : devices) {
            myprintf("Device ID:     %d\n", id);
            myprintf("Device name:   %s\n",
                     trim(d.getInfo<CL_DEVICE_NAME>()).c_str());
            myprintf("Device type:   %s\n",
                     opencl_dev_type_to_string(d.getInfo<CL_DEVICE_TYPE>()).c_str());
            myprintf("Device vendor: %s\n",
                      d.getInfo<CL_DEVICE_VENDOR>().c_str());
            myprintf("Device driver: %s\n",
                      d.getInfo<CL_DRIVER_VERSION>().c_str());
            myprintf("Device speed:  %u MHz\n",
                      d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>());
            myprintf("Device cores:  %u CU\n",
                      d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());

            // assign score, try to find best device
            int this_score = 0;
            std::string this_vendor = d.getInfo<CL_DEVICE_VENDOR>();
            this_score += 1000 * boost::icontains(this_vendor, "advanced micro devices");
            this_score += 1000 * boost::icontains(this_vendor, "amd");
            this_score += 1000 * boost::icontains(this_vendor, "nvidia");
            this_score +=  500 * boost::icontains(this_vendor, "intel");
            this_score +=  100 * (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU);
            this_score +=  opencl_version * 10;
            myprintf("Device score:  %d\n", this_score);

            bool preferred = std::find(cfg_gpus.cbegin(), cfg_gpus.cend(), id) != cfg_gpus.cend();

            if ((this_score > best_score) || preferred) {
                best_version = opencl_version;
                best_platform = p;
                best_device = d;
                if (preferred) {
                    best_score = std::numeric_limits<decltype(best_score)>::max();
                } else {
                    best_score = this_score;
                }
                found_device = true;
            }
            id++;
        }
    }

    if (!found_device) {
        throw std::runtime_error("No suitable OpenCL device found.");
    }

    cl::Platform::setDefault(best_platform);
    myprintf("Selected platform: %s\n", best_platform.getInfo<CL_PLATFORM_NAME>().c_str());
    myprintf("Selected device: %s\n", trim(best_device.getInfo<CL_DEVICE_NAME>()).c_str());
    myprintf("with OpenCL %2.1f capability\n", best_version);

    cl::Context context;
    try {
        context = cl::Context(best_device);
    } catch (const cl::Error &e) {
        myprintf("Error creating OpenCL context: %s: %d", e.what(), e.err());
        throw;
    }
    cl::Context::setDefault(context);
    cl::Device::setDefault(best_device);

    // Read source file
    //std::ifstream sourceFile("convolve_kernel.cl", std::ifstream::in);
    //std::string sourceCode(std::istreambuf_iterator<char>(sourceFile),
    //                       (std::istreambuf_iterator<char>()));

    // Make program of the source code in the context
    try {
        m_program = cl::Program(sourceCode_convolve15
                                + sourceCode_convolve3
                                + sourceCode_utility);
    } catch (const cl::Error &e) {
        myprintf("Error getting kernels: %s: %d", e.what(), e.err());
        throw;
    }
    // Build program for these specific devices
    try {
        m_program.build("-cl-mad-enable -cl-fast-relaxed-math -cl-no-signed-zeros -cl-denorms-are-zero");
    } catch (const cl::Error&) {
        myprintf("Error building kernels: %s\n",
                    m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(cl::Device::getDefault()).c_str());
        throw;
    }

    ensure_thread_initialized();

    m_wavefront_size =
        opencl_thread_data.m_convolve3_kernel.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(
            best_device);
    myprintf("Wavefront/Warp size: %d\n", m_wavefront_size);

    m_max_workgroup_size = best_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    m_max_workgroup_dims = best_device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();

    myprintf("Max workgroup size: %d\n", m_max_workgroup_size);
    myprintf("Max workgroup dimensions: ");
    for (auto d : m_max_workgroup_dims) {
        myprintf("%d ", d);
    }
    myprintf("\n");

    m_init_ok = true;
}

std::string OpenCL::get_device_name() {
    std::stringstream ss;

    cl::Device device = cl::Device::getDefault();
    ss << "OpenCL: ";
    ss << device.getInfo<CL_DEVICE_VENDOR>() << " ";
    ss << device.getInfo<CL_DEVICE_NAME>() << " @ ";
    ss << device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "MHz";

    return ss.str();
}
#endif
