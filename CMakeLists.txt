cmake_minimum_required(VERSION 3.25)
project(MorphyChess VERSION 1.0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(HPX 1.11 REQUIRED)

add_library(morphychess
    src/bits.cpp
    src/board.cpp
    src/book.cpp
    src/debug.cpp
    src/egtb.cpp
    src/evaluate.cpp
    src/history.cpp
    src/learning.cpp
    src/memory.cpp
    src/move_data.cpp
    src/node.cpp
    src/parallel_search.cpp
    src/pawn_statistics.cpp
    src/pgn.cpp
    src/rating.cpp
    src/search.cpp
    src/search_quiescent.cpp
    src/see.cpp
    src/seek_bot.cpp
    src/static_evaluator.cpp
    src/zobrist.cpp
    src/command_line.cpp
    src/bench.cpp
    src/xboard.cpp
)
add_library(MorphyChess::morphychess ALIAS morphychess)
target_include_directories(morphychess PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(morphychess PUBLIC cxx_std_23)
target_link_libraries(morphychess PUBLIC HPX::hpx)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(morphychess PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(MORPHY_SANITIZERS "" CACHE STRING "Optional compiler sanitizers, e.g. undefined")
if(MORPHY_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "MORPHY_SANITIZERS requires GCC or Clang")
    endif()
    target_compile_options(morphychess PUBLIC "-fsanitize=${MORPHY_SANITIZERS}" -fno-omit-frame-pointer)
    target_link_options(morphychess PUBLIC "-fsanitize=${MORPHY_SANITIZERS}")
endif()

add_executable(morphy src/main.cpp)
target_link_libraries(morphy PRIVATE MorphyChess::morphychess)

# The engine locates its book relative to its working directory. Preserve
# any previously learned book when reconfiguring a build directory.
foreach(data_file IN ITEMS book.bin eco.pgn)
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${data_file}")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${data_file}"
                       "${CMAKE_CURRENT_BINARY_DIR}/${data_file}" COPYONLY)
    endif()
endforeach()

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
            URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    file(GLOB test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp")
    add_executable(morphy_tests tests/test_main.cpp ${test_sources})
    target_link_libraries(morphy_tests PRIVATE MorphyChess::morphychess GTest::gtest)
    add_test(NAME unit_one_worker COMMAND morphy_tests --hpx:threads=1 --hpx:bind=none)
    add_test(NAME unit_four_workers COMMAND morphy_tests --hpx:threads=4 --hpx:bind=none)
    set_tests_properties(unit_one_worker unit_four_workers PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
    add_test(NAME version COMMAND morphy --version --hpx:threads=1 --hpx:bind=none)
    set_tests_properties(version PROPERTIES PASS_REGULAR_EXPRESSION "C\\+\\+23, HPX local runtime" TIMEOUT 20)
    add_test(NAME help COMMAND morphy --help --hpx:threads=1 --hpx:bind=none)
    set_tests_properties(help PROPERTIES PASS_REGULAR_EXPRESSION "MorphyChess options" TIMEOUT 20)
    add_test(NAME reject_unknown_option COMMAND morphy --unknown-option --hpx:threads=1 --hpx:bind=none)
    set_tests_properties(reject_unknown_option PROPERTIES WILL_FAIL TRUE TIMEOUT 20)
    add_test(NAME benchmark_smoke COMMAND morphy --bench --depth=1 --bench-seconds=0
        --threads=1 --hpx:threads=1 --hpx:bind=none)
    set_tests_properties(benchmark_smoke PROPERTIES PASS_REGULAR_EXPRESSION "Position 10/10" TIMEOUT 60)
endif()
