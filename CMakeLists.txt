cmake_minimum_required(VERSION 3.18.2)
project(leelagui VERSION 1.3.0)

if(MSVC)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17")
endif(MSVC)

set(COMP_OPT_DEBUG -DNDEBUG)
set(COMP_OPT_GIT_REVISION -DNO_GIT_REVISION)

if(USE_GPU)
  set(COMP_OPT_USE_BACKEND -DUSE_GPU -DUSE_OPENCL_BACKEND)
  set(NEURALNET_BACKEND_SOURCES cpp/neuralnet/openclbackend.cpp
                                cpp/neuralnet/openclkernels.cpp
                                cpp/neuralnet/openclhelpers.cpp
                                cpp/neuralnet/opencltuner.cpp)
else()
  set(COMP_OPT_USE_BACKEND -DUSE_EIGEN_BACKEND)
  set(NEURALNET_BACKEND_SOURCES cpp/neuralnet/eigenbackend.cpp)
endif(USE_GPU)

set(COMP_OPTS_USE_AVX2 "")
if(USE_AVX2)
  if(MSVC)
    set(COMP_OPTS_USE_AVX2 /arch:AVX2 -D__FMA__)
  else()
    set(COMP_OPTS_USE_AVX2 -mavx2 -mfma)
  endif(MSVC)
endif(USE_AVX2)

set(COMP_OPTS_USE_WLCOPY "")
if(USE_WLCOPY)
  set(COMP_OPTS_USE_WLCOPY -DUSE_WLCOPY)
endif(USE_WLCOPY)

message(STATUS "BOOST_ROOT=${BOOST_ROOT}")
set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
if (MSVC)
  set(Boost_USE_STATIC_RUNTIME ON)
else()
  set(Boost_USE_STATIC_RUNTIME OFF)
endif(MSVC)

if (MSVC)
  find_package(Boost REQUIRED)
else()
  find_package(Boost REQUIRED COMPONENTS program_options)
endif(MSVC)
message(STATUS "  Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")
message(STATUS "  Boost_LIBRARY_DIRS=${Boost_LIBRARY_DIRS}")
message(STATUS "  Boost_LIBRARIES=${Boost_LIBRARIES}")

if(USE_GPU)
  message(STATUS "OpenCL_ROOT=${OpenCL_ROOT}")
  find_package(OpenCL REQUIRED)
  message(STATUS "  OpenCL_INCLUDE_DIRS=${OpenCL_INCLUDE_DIRS}")
  message(STATUS "  OpenCL_LIBRARIES=${OpenCL_LIBRARIES}")
else()
  if (MSVC)
    message(STATUS "OpenBLAS_ROOT=${OpenBLAS_ROOT}")
    find_package(OpenBLAS REQUIRED)
  else()
    message(STATUS "BLAS_ROOT=${BLAS_ROOT}")
    find_package(BLAS REQUIRED)
  endif(MSVC)
  message(STATUS "  BLAS_INCLUDE_DIRS=${BLAS_INCLUDE_DIRS}")
  message(STATUS "  BLAS_LIBRARIES=${BLAS_LIBRARIES}")

  message(STATUS "EIGEN3_ROOT=${EIGEN3_ROOT}")
  if (MSVC)
     set(EIGEN3_INCLUDE_DIRS "$ENV{EIGEN3_INCLUDE_DIR}")
  else()
     find_package(Eigen3 REQUIRED)
  endif(MSVC)
  message(STATUS "  EIGEN3_INCLUDE_DIRS=${EIGEN3_INCLUDE_DIRS}")
endif(USE_GPU)

message(STATUS "ZLIB_ROOT=${ZLIB_ROOT}")
find_package(ZLIB)
if(ZLIB_FOUND)
  message(STATUS "  ZLIB_INCLUDE_DIRS=${ZLIB_INCLUDE_DIRS}")
  message(STATUS "  ZLIB_LIBRARY_DIRS=${ZLIB_LIBRARY_DIRS}")
  message(STATUS "  ZLIB_LIBRARIES=${ZLIB_LIBRARIES}")
endif(ZLIB_FOUND)

find_library(LIBZIP_LIBRARY NAMES zip)
find_path(LIBZIP_INCLUDE_DIR_ZIP NAMES zip.h)
find_path(LIBZIP_INCLUDE_DIR_ZIPCONF NAMES zipconf.h)
if(LIBZIP_LIBRARY AND LIBZIP_INCLUDE_DIR_ZIP AND  LIBZIP_INCLUDE_DIR_ZIPCONF)
  message(STATUS "  LIBZIP_INCLUDE_DIR_ZIP=${LIBZIP_INCLUDE_DIR_ZIP}")
  message(STATUS "  LIBZIP_INCLUDE_DIR_ZIPCONF=${LIBZIP_INCLUDE_DIR_ZIPCONF}")
  message(STATUS "  LIBZIP_LIBRARY=${LIBZIP_LIBRARY}")
endif()

set(COMP_OPTS_USE_TCMALLOC "")
if(USE_TCMALLOC)
  find_library(TCMALLOC_LIB NAMES tcmalloc_minimal HINTS /usr)
  if(TCMALLOC_LIB)
    message(STATUS "  TCMALLOC_LIB=${TCMALLOC_LIB}")
    set(COMP_OPTS_USE_TCMALLOC -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free)
  endif(TCMALLOC_LIB)
endif(USE_TCMALLOC)

message(STATUS "wxWidgets_ROOT=${wxWidgets_ROOT}")
if (MSVC)
  find_package(wxWidgets REQUIRED OPTIONAL_COMPONENTS core base qa xrc)
else()
  find_package(wxWidgets COMPONENTS core base qa xrc)
endif(MSVC)
message(STATUS "  wxWidgets_USE_FILE=${wxWidgets_USE_FILE}")
message(STATUS "  wxWidgets_LIBRARIES=${wxWidgets_LIBRARIES}")

include(${wxWidgets_USE_FILE})

if(NOT MSVC)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  message(STATUS "  GTK3_INCLUDE_DIRS=${GTK3_INCLUDE_DIRS}")
  message(STATUS "  GTK3_LIBRARY_DIRS=${GTK3_LIBRARY_DIRS}")
  message(STATUS "  GTK3_LIBRARIES=${GTK3_LIBRARIES}")
  message(STATUS "  GTK3_CFLAGS_OTHER=${GTK3_CFLAGS_OTHER}")
endif()

set(INCLUDE_DIRS
  "./engine"
  "./cpp"
  "./cpp/external"
  "./cpp/external/tclap-1.2.2/include"
  "./cpp/external/filesystem-1.5.8/include"
  "${Boost_INCLUDE_DIRS}"
  "${OpenCL_INCLUDE_DIRS}"
  "${BLAS_INCLUDE_DIRS}"
  "${EIGEN3_INCLUDE_DIRS}"
  "${ZLIB_INCLUDE_DIRS}"
  "${LIBZIP_INCLUDE_DIR_ZIP}"
  "${LIBZIP_INCLUDE_DIR_ZIPCONF}"
)

if(MSVC)
  set(INCLUDE_DIRS ${INCLUDE_DIRS} "./Win32")
endif(MSVC)
include_directories(${INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIRS} ${ZLIB_LIBRARIES} ${LIBZIP_LIBRARY})

set(SRCS
  engine/Network.cpp
  engine/AttribScores.cpp
  engine/FullBoard.cpp
  engine/KoState.cpp
  engine/Playout.cpp
  engine/TimeControl.cpp
  engine/UCTSearch.cpp
  engine/Attributes.cpp
  engine/Book.cpp
  engine/GameState.cpp
  engine/PNNode.cpp
  engine/SGFParser.cpp
  engine/Timing.cpp
  engine/Utils.cpp
  engine/FastBoard.cpp
  engine/Matcher.cpp
  engine/PNSearch.cpp
  engine/SGFTree.cpp
  engine/TTable.cpp
  engine/Zobrist.cpp
  engine/FastState.cpp
  engine/GTP.cpp
  engine/MCOTable.cpp
  engine/Random.cpp
  engine/SMP.cpp
  engine/UCTNode.cpp
  engine/NN.cpp
  engine/NN128.cpp
  engine/NNValue.cpp
  engine/OpenCL.cpp
  engine/MCPolicy.cpp
  cpp/book/bookcssjs.cpp
  cpp/book/bookKata.cpp
  cpp/command/commandline.cpp
  cpp/command/gtpKata.cpp
  cpp/core/base64.cpp
  cpp/core/bsearch.cpp
  cpp/core/commandloop.cpp
  cpp/core/config_parser.cpp
  cpp/core/datetime.cpp
  cpp/core/elo.cpp
  cpp/core/fancymath.cpp
  cpp/core/fileutils.cpp
  cpp/core/global.cpp
  cpp/core/hash.cpp
  cpp/core/logger.cpp
  cpp/core/mainargs.cpp
  cpp/core/makedir.cpp
  cpp/core/md5.cpp
  cpp/core/multithread.cpp
  cpp/core/rand.cpp
  cpp/core/rand_helpers.cpp
  cpp/core/sha2.cpp
  cpp/core/test.cpp
  cpp/core/threadsafecounter.cpp
  cpp/core/threadsafequeue.cpp
  cpp/core/threadtest.cpp
  cpp/core/threadtest.h
  cpp/core/timer.cpp
  cpp/dataio/files.cpp
  cpp/dataio/homedata.cpp
  cpp/dataio/loadmodel.cpp
  cpp/dataio/numpywrite.cpp
  cpp/dataio/sgf.cpp
  cpp/dataio/trainingwrite.cpp
  cpp/game/board.cpp
  cpp/game/boardhistory.cpp
  cpp/game/graphhash.cpp
  cpp/game/rules.cpp
  cpp/neuralnet/desc.cpp
  cpp/neuralnet/modelversion.cpp
  cpp/neuralnet/nneval.cpp
  cpp/neuralnet/nninputs.cpp
  ${NEURALNET_BACKEND_SOURCES}
  cpp/program/gtpconfig.cpp
  cpp/program/play.cpp
  cpp/program/playsettings.cpp
  cpp/program/playutils.cpp
  cpp/program/selfplaymanager.cpp
  cpp/program/setup.cpp
  cpp/search/analysisdata.cpp
  cpp/search/asyncbot.cpp
  cpp/search/distributiontable.cpp
  cpp/search/localpattern.cpp
  cpp/search/mutexpool.cpp
  cpp/search/patternbonustable.cpp
  cpp/search/reportedsearchvalues.cpp
  cpp/search/search.cpp
  cpp/search/searchexplorehelpers.cpp
  cpp/search/searchhelpers.cpp
  cpp/search/searchmirror.cpp
  cpp/search/searchmultithreadhelpers.cpp
  cpp/search/searchnnhelpers.cpp
  cpp/search/searchnode.cpp
  cpp/search/searchnodetable.cpp
  cpp/search/searchparams.cpp
  cpp/search/searchprint.cpp
  cpp/search/searchpuct.cpp
  cpp/search/searchresults.cpp
  cpp/search/searchtimehelpers.cpp
  cpp/search/searchupdatehelpers.cpp
  cpp/search/subtreevaluebiastable.cpp
  cpp/search/timecontrols.cpp
  cpp/tests/testboardarea.cpp
  cpp/tests/testboardbasic.cpp
  cpp/tests/testcommon.cpp
  cpp/tests/testnn.cpp
  cpp/tests/testnnevalcanary.cpp
  cpp/tests/testnninputs.cpp
  cpp/tests/testownership.cpp
  cpp/tests/testrules.cpp
  cpp/tests/testscore.cpp
  cpp/tests/testsearch.cpp
  cpp/tests/testsearchcommon.cpp
  cpp/tests/testsearchmisc.cpp
  cpp/tests/testsearchnonn.cpp
  cpp/tests/testsearchv3.cpp
  cpp/tests/testsearchv8.cpp
  cpp/tests/testsearchv9.cpp
  cpp/tests/testsgf.cpp
  cpp/tests/testsymmetries.cpp
  cpp/tests/testtime.cpp
  cpp/tests/testtrainingwrite.cpp
  cpp/tests/tinymodel.cpp
  cpp/tests/tinymodeldata.cpp
  AboutDialog.cpp
  AnalysisWindow.cpp
  App.cpp
  CalculateDialog.cpp
  ClockAdjustDialog.cpp
  EngineThread.cpp
  GUI.cpp
  MainFrame.cpp
  NewGameDialog.cpp
  RatedSizeDialog.cpp
  ScoreDialog.cpp
  ScoreHistogram.cpp
  SettingsDialog.cpp
  TBoardPanel.cpp
  TScorePanel.cpp
)
if(MSVC)
  set(SRCS ${SRCS} Win32/LeelaGUI.rc Win32/resource.h)
endif(MSVC)

add_executable(leelagui WIN32 ${SRCS})

if(NOT MSVC)
  target_include_directories(leelagui PUBLIC ${GTK3_INCLUDE_DIRS})
  target_link_directories(leelagui PUBLIC ${GTK3_LIBRARY_DIRS})
  add_definitions(${GTK3_CFLAGS_OTHER})
endif()

target_compile_features(leelagui PUBLIC cxx_std_17)

if(MSVC)
  set_property(TARGET leelagui PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_link_libraries(leelagui PUBLIC
    imagehlp.lib
    dbghelp.lib
    ${Boost_LIBRARIES}
    ${OpenCL_LIBRARIES}
    ${BLAS_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBZIP_LIBRARY}
    ${TCMALLOC_LIBRARY}
  )
  target_compile_options(leelagui PUBLIC
    ${COMP_OPTS_USE_AVX2}
    ${COMP_OPTS_USE_TCMALLOC}
  )
  target_compile_definitions(leelagui PUBLIC
    BOOST_ALL_NO_LIB
    WIN32
    _CRT_SECURE_NO_DEPRECATE=1
    _CRT_NON_CONFORMING_SWPRINTFS=1
    _SCL_SECURE_NO_WARNINGS=1
    __WXMSW__
    _UNICODE
    _WINDOWS
    NOPCH
    WINVER=0x0501
    wxUSE_GUI=1
    _SECURE_SCL=0
    BYTE_ORDER=1234
    LITTLE_ENDIAN=1234
    BIG_ENDIAN=4321
    ${COMP_OPT_DEBUG}
    ${COMP_OPT_GIT_REVISION}
    ${COMP_OPT_USE_BACKEND}
    ${COMP_OPTS_USE_WLCOPY}
    ${COMP_OPTS_USE_AVX2}
    ${COMP_OPTS_USE_TCMALLOC}
  )
else()
  target_link_libraries(leelagui PUBLIC
    -Wl,--as-needed,-Bdynamic
#    -fopenmp
    -lpthread
    -lm
    ${Boost_LIBRARIES}
    ${OpenCL_LIBRARIES}
    ${BLAS_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBZIP_LIBRARY}
    ${TCMALLOC_LIBRARY}
  )
  target_compile_options(leelagui PUBLIC
    -O3
    -ffast-math
    -march=native
    -mtune=generic
#    -fopenmp
#    -pedantic
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-sign-compare
    -Wcast-align
#    -Wcast-qual
#    -Wctor-dtor-privacy
    -Wdisabled-optimization
    -Wformat=2
#    -Wmissing-declarations
    -Wmissing-include-dirs
    -Woverloaded-virtual
    -Wredundant-decls
#    -Wshadow
    -Wstrict-overflow=1
#    -Wswitch-default
#    -Wfloat-conversion
    -Wunused
    -Wlogical-op
#    -Wnoexcept
    -Wstrict-null-sentinel
    -Wno-maybe-uninitialized
    -Wno-unused-function
    -Wno-enum-compare
    ${COMP_OPTS_USE_AVX2}
    ${COMP_OPTS_USE_TCMALLOC}
  )
  
  target_compile_definitions(leelagui PUBLIC
    ${COMP_OPT_DEBUG}
    ${COMP_OPT_GIT_REVISION}
    ${COMP_OPT_USE_BACKEND}
    ${COMP_OPTS_USE_WLCOPY}
  )
endif(MSVC)
