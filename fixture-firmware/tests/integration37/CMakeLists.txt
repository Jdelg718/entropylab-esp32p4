cmake_minimum_required(VERSION 3.16)
project(lifehash37_focused C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(../../integrated-lanes/lifehash/core lifehash-core)
add_executable(lifehash_application_test
    lifehash_application_test.cpp
    ../../app/main/all_features_application.cpp
    ../../app/main/all_features_dispatch.cpp)
target_include_directories(lifehash_application_test PRIVATE
    ../../app/main
    ../../integrated-lanes/playingcards/adapter/include
    ../../integrated-lanes/lifehash/core/include)
target_link_libraries(lifehash_application_test PRIVATE entropylab_lifehash)
target_compile_options(lifehash_application_test PRIVATE
    -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
