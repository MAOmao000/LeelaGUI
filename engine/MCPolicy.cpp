#include "config.h"

#include <memory>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <string.h>
#include <map>
#include <unordered_map>
#if defined(_OPENMP)
#include <omp.h>
#endif
#include "GTP.h"
#include "MCPolicy.h"
#include "SGFParser.h"
#include "SGFTree.h"
#include "Utils.h"
#include "Random.h"
#include "Network.h"
#include "Playout.h"

using namespace Utils;

#include "PolicyWeights.h"
#include "PolicyWeightsSL.h"
#include "PatternsLive.h"
alignas(64) std::array<float, NUM_PATTERNS> PolicyWeights::pattern_gradients;
alignas(64) std::array<float, NUM_FEATURES> PolicyWeights::feature_gradients;
// Filled by Matcher.cpp from PolicyWeights::live_patterns
// alignas(64) const std::array<float, NUM_PATTERNS> PolicyWeights::pattern_weights_sl;

// Adam
alignas(64) std::array<std::pair<float, float>, NUM_PATTERNS> pattern_adam{};
alignas(64) std::array<std::pair<float, float>, NUM_FEATURES> feature_adam{};
int t{0};


void MCPolicy::mse_from_file(std::string filename) {
    std::vector<std::string> games = SGFParser::chop_all(filename);
    size_t gametotal = games.size();
    myprintf("Total games in file: %d\n", gametotal);

#if defined(_OPENMP)
    omp_set_num_threads(cfg_num_threads);
#endif

    double sum_sq_pp = 0.0;
    //double sum_sq_nn = 0.0;
    int count = 0;

    PolicyWeights::feature_weights.fill(1.0f);
    PolicyWeights::pattern_weights.fill(1.0f);
    Time start;

    while (1) {
        size_t pick = Random::get_Rng()->randuint32(gametotal);

        std::unique_ptr<SGFTree> sgftree(new SGFTree);
        try {
            sgftree->load_from_string(games[pick]);
        } catch (...) {
        };

        int who_won = sgftree->get_winner();
        int handicap = sgftree->get_state()->get_handicap();

        int movecount = sgftree->count_mainline_moves();
        int move_pick = Random::get_Rng()->randuint16(movecount);
        // GameState state = sgftree->follow_mainline_state(move_pick);
        KoState * state = sgftree->get_state_from_mainline(move_pick);

        if (who_won != FastBoard::BLACK && who_won != FastBoard::WHITE) {
            continue;
        }
        bool blackwon = (who_won == FastBoard::BLACK);

        constexpr int iterations = 512;
        PolicyWeights::feature_gradients.fill(0.0f);
        PolicyWeights::pattern_gradients.fill(0.0f);
        float bwins = 0.0f;
        //float nwscore;
        float black_score = blackwon ? 1.0f : 0.0f;

        #pragma omp parallel
        {
#if 0
            #pragma omp single nowait
            {
                FastState workstate = *state;
                nwscore = Network::get_Network()->get_value(
                    &workstate, Network::Ensemble::AVERAGE_ALL);
                if (workstate.get_to_move() == FastBoard::WHITE) {
                    nwscore = 1.0f - nwscore;
                }
                black_score = ((blackwon ? 1.0f : 0.0f) + nwscore) / 2.0f;
            }
#endif
#if 1
            // Get EV (V)
            #pragma omp for reduction(+:bwins) schedule(dynamic, 8)
            for (int i = 0; i < iterations; i++) {
                FastState tmp = *state;

                Playout p;
                p.run(tmp, false, true, nullptr);

                float score = p.get_score();
                if (score > 0.0f) {
                    bwins += 1.0f / (float)iterations;
                }
            }
#endif
#if 1
            // Policy Trace per thread
            #pragma omp for schedule(dynamic, 4)
            for (int i = 0; i < iterations; i++) {
                FastState tmp = *state;

                PolicyTrace policy_trace;
                Playout p;
                p.run(tmp, false, true, &policy_trace);

                bool black_won = p.get_score() > 0.0f;
                policy_trace.trace_process(iterations, bwins, black_won);
            }
#endif
        }

        MCPolicy::adjust_weights(black_score, bwins);
        //MCPolicy::adjust_weights(1.0f, 0.0f);

       // myprintf("n=%d BW: %d Score: %1.4f NN: %1.4f ",
       //          count, blackwon, bwins, nwscore);

        sum_sq_pp += std::pow((blackwon ? 1.0f : 0.0f) - bwins,   2.0f);
//        sum_sq_nn += std::pow((blackwon ? 1.0f : 0.0f) - nwscore, 2.0f);

        count++;

        if (count % 10000 == 0) {
            Time end;
            float timediff = Time::timediff(start, end) / 100.0f;
            float ips = 10000.0f / timediff;
            start = end;
            myprintf("n=%d MSE MC=%1.4f ips=%f\n",
                count,
                sum_sq_pp/10000.0,
  //              sum_sq_nn/10000.0,
                ips);
            sum_sq_pp = 0.0;
//            sum_sq_nn = 0.0;
        }

        if (count % 10000 == 0) {
            std::string filename = "rltune_" + std::to_string(count) + ".txt";
            std::ofstream out(filename);
            out.precision(std::numeric_limits<float>::max_digits10);
            out << std::defaultfloat;
            for (int w = 0; w < NUM_FEATURES; w++) {
                out << PolicyWeights::feature_weights[w]
                    << "f, /* = " << w << " */"
                    << std::endl;
            }
            out << std::endl;
            out << std::scientific;
            //std::map<int, float> active_pats;
            for (size_t i = 0; i < PolicyWeights::pattern_weights.size(); i++) {
                //if  (PolicyWeights::pattern_weights[i] != 1.0f) {
                //    active_pats.emplace(i, PolicyWeights::pattern_weights[i]);
                //}
            //}
            //for (auto & pat : active_pats) {
                out << PolicyWeights::pattern_weights[i] << "f," << std::endl;
            }
            out.close();
        }
    }
}

void MCPolicy::mse_from_file2(std::string filename) {
    std::vector<std::string> games = SGFParser::chop_all(filename);
    size_t gametotal = games.size();
    myprintf("Total games in file: %d\n", gametotal);

#if defined(_OPENMP)
    omp_set_num_threads(cfg_num_threads);
#endif

    int count = 0;
    int picks = 0;
    int correct = 0;

    PolicyWeights::feature_weights.fill(1.0f);
    PolicyWeights::pattern_weights.fill(1.0f);

    PolicyWeights::feature_gradients.fill(0.0f);
    PolicyWeights::pattern_gradients.fill(0.0f);

    Time start;

    for (;;) {
        #pragma omp parallel for
        for (int gameid = 0; gameid < 128; gameid++) {
            std::unique_ptr<SGFTree> sgftree(new SGFTree);
            try {
                sgftree->load_from_string(games[Random::get_Rng()->randuint32(gametotal)]);
            } catch (...) {
                #pragma omp atomic
                count++;
                continue;
            };

            SGFTree * treewalk = &(*sgftree);
            size_t counter = 0;
            size_t movecount = sgftree->count_mainline_moves();
            std::vector<int> tree_moves = sgftree->get_mainline();

            PolicyTrace policy_trace;

            while (counter < movecount) {
                KoState * state = treewalk->get_state();
                int tomove = state->get_to_move();
                int move;

                if (treewalk->get_child(0) != NULL) {
                    move = treewalk->get_child(0)->get_move(tomove);
                    if (move == SGFTree::EOT) {
                        break;
                    }
                } else {
                    break;
                }
                assert(move == tree_moves[counter]);

                state->generate_trace(tomove, policy_trace, move);

                counter++;
                treewalk = treewalk->get_child(0);
            }

            policy_trace.accumulate_sl_gradient(correct, picks);

            #pragma omp atomic
            count++;
        }

        MCPolicy::adjust_weights(1.0f, 0.0f);
        PolicyWeights::feature_gradients.fill(0.0f);
        PolicyWeights::pattern_gradients.fill(0.0f);

        std::cout << ".";

        if ((count % (128*128)) == 0) {
            std::cout << std::endl << "Pred: " << correct*100.0/picks;
            picks = 0;
            correct = 0;

            std::cout << "w";
            std::string filename = "sltune_" + std::to_string(count) + ".txt";
            std::ofstream out(filename);
            out.precision(std::numeric_limits<float>::max_digits10);
            out << std::defaultfloat;
            for (int w = 0; w < NUM_FEATURES; w++) {
                out << PolicyWeights::feature_weights[w]
                    << "f, /* = " << w << " */"
                    << std::endl;
            }
            out << std::endl;
            out << std::scientific;
            for (int w = 0; w < NUM_PATTERNS; w++) {
                out << PolicyWeights::pattern_weights[w] << "f," << std::endl;
            }
            out.close();
        }
    }
}

void PolicyTrace::accumulate_sl_gradient(int & correct, int & picks) {
    if (trace.empty()) return;

    float positions = trace.size();
    assert(positions > 0.0f);
    float factor = 1.0f / positions;

    alignas(64) std::array<float, NUM_FEATURES> policy_feature_gradient{};
    std::vector<float> candidate_scores;
    std::vector<int> patterns;

    for (auto & decision : trace) {
        candidate_scores.clear();
        candidate_scores.reserve(decision.candidates.size());
        // get real probabilities
        float sum_scores = 0.0f;
        float max_score = 0.0f;
        int max_move = -1;
        for (auto & mwf : decision.candidates) {
            float score = mwf.get_score();
            sum_scores += score;
            if (score > max_score) {
                max_score = score;
                max_move = mwf.get_sq();
            }
            assert(!std::isnan(score));
            candidate_scores.push_back(score);
        }
        assert(sum_scores > 0.0f);
        assert(!std::isnan(sum_scores));
        if (max_move == decision.pick.get_sq()) {
            #pragma omp atomic
            correct++;
        }
        #pragma omp atomic
        picks++;

        // transform to probabilities
        for (float & score : candidate_scores) {
            score /= sum_scores;
        }

        // loop over features, get prob of feature
        alignas(64) std::array<float, NUM_FEATURES> feature_probabilities{};
        for (size_t c = 0; c < candidate_scores.size(); c++) {
            float candidate_probability = candidate_scores[c];
            MovewFeatures & mwf = decision.candidates[c];
            for (int i = 0; i < NUM_FEATURES; i++) {
                if (mwf.has_flag(i)) {
                    feature_probabilities[i] += candidate_probability;
                }
            }
        }

        // get policy gradient
        for (int i = 0; i < NUM_FEATURES; i++) {
            float observed = 0.0f;
            if (decision.pick.has_flag(i)) {
                observed = 1.0f;
            }
            policy_feature_gradient[i] += (observed - feature_probabilities[i]);
            assert(!std::isnan(policy_feature_gradient[i]));
        }

        patterns.clear();
        // get pat probabilities
        std::array<float, NUM_PATTERNS> pattern_probabilities;
        for (size_t c = 0; c < candidate_scores.size(); c++) {
            if (!decision.candidates[c].is_pass()) {
                int pat = decision.candidates[c].get_pattern();
                if (std::find(patterns.cbegin(), patterns.cend(), pat)
                    != patterns.cend()) {
                    pattern_probabilities[pat] += candidate_scores[c];
                } else {
                    patterns.push_back(pat);
                    pattern_probabilities[pat]  = candidate_scores[c];
                }
            }
        }

        for (int pat : patterns) {
            float observed = 0.0f;
            if (!decision.pick.is_pass()) {
                if (decision.pick.get_pattern() == pat) {
                    observed = 1.0f;
                }
            }
            float grad = factor * (observed - pattern_probabilities[pat]);
            #pragma omp atomic
            PolicyWeights::pattern_gradients[pat] += grad;
        }
    }

    for (int i = 0; i < NUM_FEATURES; i++) {
        float grad = policy_feature_gradient[i] * factor;
        // accumulate total
        #pragma omp atomic
        PolicyWeights::feature_gradients[i] += grad;
    }
}

void PolicyTrace::trace_process(const int iterations, const float baseline,
                                const bool blackwon) {
#if 1
    float z = 1.0f;
    if (!blackwon) {
        z = 0.0f;
    }
    float sign = z - baseline;
#endif

    if (trace.empty()) return;

    float positions = trace.size();
    float iters = iterations;
    assert(positions > 0.0f && iters > 0.0f);
    // scale by N*T
    float factor = 1.0f / (positions * iters);

    alignas(64) std::array<float, NUM_FEATURES> policy_feature_gradient{};
    //alignas(64) std::array<float, NUM_PATTERNS> policy_pattern_gradient{};
    std::vector<float> candidate_scores;
    std::vector<int> patterns;

    for (auto & decision : trace) {
#if 0
        float z = 0.0f;
        // Side to move won
        if (decision.black_to_move == blackwon) {
            z = 1.0f;
        }
        // Baseline for side to move
        float adj_baseline = baseline;
        if (!decision.black_to_move) {
            adj_baseline = 1.0f - baseline;
        }
        float sign = z - adj_baseline;
#endif
        candidate_scores.clear();
        candidate_scores.reserve(decision.candidates.size());
        // get real probabilities
        float sum_scores = 0.0f;
        for (auto & mwf : decision.candidates) {
            float score = mwf.get_score();
            sum_scores += score;
            assert(!std::isnan(score));
            candidate_scores.push_back(score);
        }
        assert(sum_scores > 0.0f);
        assert(!std::isnan(sum_scores));
        // transform to probabilities
        for (float & score : candidate_scores) {
            score /= sum_scores;
        }

        // loop over features, get prob of feature
        alignas(64) std::array<float, NUM_FEATURES> feature_probabilities{};
        for (size_t c = 0; c < candidate_scores.size(); c++) {
            float candidate_probability = candidate_scores[c];
            MovewFeatures & mwf = decision.candidates[c];
            for (int i = 0; i < NUM_FEATURES; i++) {
                if (mwf.has_flag(i)) {
                    feature_probabilities[i] += candidate_probability;
                }
            }
        }

        // get policy gradient
        for (int i = 0; i < NUM_FEATURES; i++) {
            float observed = 0.0f;
            if (decision.pick.has_flag(i)) {
                observed = 1.0f;
            }
            policy_feature_gradient[i] += sign *
                (observed - feature_probabilities[i]);
            assert(!std::isnan(policy_feature_gradient[i]));
        }

        patterns.clear();
        // get pat probabilities
        std::array<float, NUM_PATTERNS> pattern_probabilities;
        //std::unordered_map<int, float> pattern_probabilities;
        for (size_t c = 0; c < candidate_scores.size(); c++) {
            if (!decision.candidates[c].is_pass()) {
                int pat = decision.candidates[c].get_pattern();
                if (std::find(patterns.cbegin(), patterns.cend(), pat)
                    != patterns.cend()) {
                    pattern_probabilities[pat] += candidate_scores[c];
                } else {
                    patterns.push_back(pat);
                    pattern_probabilities[pat]  = candidate_scores[c];
                }
            }
        }

        for (int pat : patterns) {
            float observed = 0.0f;
            if (!decision.pick.is_pass()) {
                if (decision.pick.get_pattern() == pat) {
                    observed = 1.0f;
                }
            }
            float grad = factor * sign *
                (observed - pattern_probabilities[pat]);
            #pragma omp atomic
            PolicyWeights::pattern_gradients[pat] += grad;
        }
    }

    for (int i = 0; i < NUM_FEATURES; i++) {
        float grad = policy_feature_gradient[i] * factor;
        // accumulate total
        #pragma omp atomic
        PolicyWeights::feature_gradients[i] += grad;
    }
}

void MCPolicy::adjust_weights(float black_eval, float black_winrate) {
    constexpr float alpha = 0.002f;
    constexpr float beta_1 = 0.9f;
    constexpr float beta_2 = 0.999f;
    constexpr float delta = 1e-8f;
    constexpr float lambda = 2e-7;

    // Timestep for Adam (total updates)
    t++;

    float Vdelta = black_eval - black_winrate;

    for (int i = 0; i < NUM_FEATURES; i++) {
        float orig_weight = PolicyWeights::feature_weights[i];
        float gradient = PolicyWeights::feature_gradients[i];

        feature_adam[i].first  = beta_1 * feature_adam[i].first
                                 + (1.0f - beta_1) * gradient;
        feature_adam[i].second = beta_2 * feature_adam[i].second
                                 + (1.0f - beta_2) * gradient * gradient;
        float bc_m1 = feature_adam[i].first  / (1.0f - (float)std::pow(beta_1, (double)t));
        float bc_m2 = feature_adam[i].second / (1.0f - (float)std::pow(beta_2, (double)t));
        float bc_m1_nag = beta_1 * bc_m1
                          + gradient * (1.0f - beta_1) / (1.0f - (float)std::pow(beta_1, (double)t));
        float adam_grad = alpha * bc_m1_nag / (std::sqrt(bc_m2) + delta);

        // Convert to theta
        float theta = std::log(orig_weight);
        theta -= std::abs(Vdelta) * theta * lambda;
        theta += Vdelta * adam_grad;
        float gamma = std::exp(theta);
        gamma = std::max(gamma, 1e-12f);
        gamma = std::min(gamma, 1e12f);
        assert(!std::isnan(gamma));
        PolicyWeights::feature_weights[i] = gamma;
    }

    // Give the vectorizer a chance
    alignas(64) std::array<float, NUM_PATTERNS> adam_grad;
    //std::unique_ptr<float[]> adam_grad(new float[NUM_PATTERNS]);
    #pragma omp parallel for
    for (int i = 0; i < NUM_PATTERNS; i++) {
        float gradient = PolicyWeights::pattern_gradients[i];
        pattern_adam[i].first  = beta_1 * pattern_adam[i].first
                                 + (1.0f - beta_1) * gradient;
        pattern_adam[i].second = beta_2 * pattern_adam[i].second
                                 + (1.0f - beta_2) * gradient * gradient;
        float bc_m1 = pattern_adam[i].first  / (1.0f - (float)std::pow(beta_1, (double)t));
        float bc_m2 = pattern_adam[i].second / (1.0f - (float)std::pow(beta_2, (double)t));
        float bc_m1_nag = beta_1 * bc_m1
                          + gradient * (1.0f - beta_1) / (1.0f - (float)std::pow(beta_1, (double)t));
        adam_grad[i] = alpha * bc_m1_nag / (std::sqrt(bc_m2) + delta);
    }

    #pragma omp parallel for
    for (int i = 0; i < NUM_PATTERNS; i++) {
        float orig_weight = PolicyWeights::pattern_weights[i];
        // Convert to theta
        float theta = std::log(orig_weight);
        theta -= std::abs(Vdelta) * theta * lambda;
        theta += Vdelta * adam_grad[i];
        float gamma = std::exp(theta);
        gamma = std::max(gamma, 1e-12f);
        gamma = std::min(gamma, 1e12f);
        assert(!std::isnan(gamma));
        PolicyWeights::pattern_weights[i] = gamma;
    }
}
