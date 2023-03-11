cmake_minimum_required(VERSION 3.15)
project(leelagui VERSION 1.1.0)

if(MSVC)
  cmake_policy(SET CMP0091 NEW)
  string(REPLACE "/W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /wd\"4100\" /wd\"4305\"")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DBUILD_TYPE=\\\"Debug\\\"" "/MTd")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /DBUILD_TYPE=\\\"Release\\\"" "/MT")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif(MSVC)

set(COMP_OPTS "NDEBUG")
if(USE_GPU)
  set(COMP_OPTS ${COMP_OPTS} "USE_GPU")
endif(USE_GPU)
if(USE_WLCOPY)
  set(COMP_OPTS ${COMP_OPTS} "USE_WLCOPY")
endif(USE_WLCOPY)
if(USE_THREAD)
  set(COMP_OPTS ${COMP_OPTS} "USE_THREAD")
endif(USE_THREAD)

message(STATUS "BOOST_ROOT=${BOOST_ROOT}")
message(STATUS "OpenCL_ROOT=${OpenCL_ROOT}")
set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
if (MSVC)
  set(Boost_USE_STATIC_RUNTIME ON)
else()
  set(Boost_USE_STATIC_RUNTIME OFF)
endif(MSVC)

find_package(Boost REQUIRED COMPONENTS program_options)
message(STATUS "  Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")
message(STATUS "  Boost_LIBRARY_DIRS=${Boost_LIBRARY_DIRS}")
message(STATUS "  Boost_LIBRARIES=${Boost_LIBRARIES}")

if(USE_GPU)
  find_package(OpenCL REQUIRED)
  message(STATUS "  OpenCL_INCLUDE_DIRS=${OpenCL_INCLUDE_DIRS}")
  message(STATUS "  OpenCL_LIBRARIES=${OpenCL_LIBRARIES}")
else()
  find_package(BLAS REQUIRED)
  message(STATUS "  BLAS_INCLUDE_DIRS=${BLAS_INCLUDE_DIRS}")
  message(STATUS "  BLAS_LIBRARIES=${BLAS_LIBRARIES}")
endif(USE_GPU)

find_package(wxWidgets COMPONENTS core base qa xrc)
message(STATUS "  wxWidgets_USE_FILE=${wxWidgets_USE_FILE}")
message(STATUS "  wxWidgets_LIBRARIES=${wxWidgets_LIBRARIES}")

include(${wxWidgets_USE_FILE})

if(NOT MSVC)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 gtk+-3.0)
  if(NOT GTK3_FOUND)
    pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
    message(STATUS "  GTK_INCLUDE_DIRS=${GTK2_INCLUDE_DIRS}")
    message(STATUS "  GTK_LIBRARY_DIRS=${GTK2_LIBRARY_DIRS}")
    message(STATUS "  GTK_LIBRARIES=${GTK2_LIBRARIES}")
    message(STATUS "  GTK_CFLAGS_OTHER=${GTK2_CFLAGS_OTHER}")
  else()
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    message(STATUS "  GTK_INCLUDE_DIRS=${GTK3_INCLUDE_DIRS}")
    message(STATUS "  GTK_LIBRARY_DIRS=${GTK3_LIBRARY_DIRS}")
    message(STATUS "  GTK_LIBRARIES=${GTK3_LIBRARIES}")
    message(STATUS "  GTK_CFLAGS_OTHER=${GTK3_CFLAGS_OTHER}")
  endif()
endif()

set(INCLUDE_DIRS
  "./engine"
  "${Boost_INCLUDE_DIRS}"
  "${OpenCL_INCLUDE_DIRS}"
  "${BLAS_INCLUDE_DIRS}"
)
if(MSVC)
  set(INCLUDE_DIRS ${INCLUDE_DIRS} "./Win32")
endif(MSVC)
include_directories(${INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIRS})

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
  SubProcess.cpp
  TBoardPanel.cpp
  TScorePanel.cpp
)
if(MSVC)
  set(SRCS ${SRCS} Win32/LeelaGUI.rc Win32/resource.h)
endif(MSVC)

add_executable(leelagui WIN32 ${SRCS})

if(NOT MSVC)
  target_include_directories(leelagui PUBLIC ${GTK_INCLUDE_DIRS})
  target_link_directories(leelagui PUBLIC ${GTK_LIBRARY_DIRS})
  add_definitions(${GTK_CFLAGS_OTHER})
endif()

target_compile_features(leelagui PUBLIC cxx_std_17)
if(MSVC)
  set_property(TARGET leelagui PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Debug>")
  target_compile_definitions(leelagui PUBLIC
    ${COMP_OPTS}
    BOOST_ALL_NO_LIB
    WIN32
    NDEBUG
    _WINDOWS
    __WXMSW__
    WINVER=0x0501
    wxUSE_GUI=1
    _SECURE_SCL=0
    _ITERATOR_DEBUG_LEVEL=0
  )
  target_link_libraries(leelagui PUBLIC
    imagehlp.lib
    dbghelp.lib
    ${Boost_LIBRARIES}
    ${OpenCL_LIBRARIES}
    ${BLAS_LIBRARIES}
    ${wxWidgets_LIBRARIES}
  )  
else()
  target_link_libraries(leelagui PUBLIC
    -Wl,--as-needed,-Bdynamic
    -lpthread
    -lm
    ${Boost_LIBRARIES}
    ${OpenCL_LIBRARIES}
    ${BLAS_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GTK_LIBRARIES}
  )
  target_compile_options(leelagui PUBLIC
    -O3
    -ffast-math
    -march=native
    -mtune=generic
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unknown-pragmas
    -Wno-trigraphs
  )
  
  target_compile_definitions(leelagui PUBLIC
    ${COMP_OPTS}
  )
endif(MSVC)