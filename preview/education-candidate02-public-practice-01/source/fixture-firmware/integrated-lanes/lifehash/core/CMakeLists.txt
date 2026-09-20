cmake_minimum_required(VERSION 3.16)
project(entropylab_lifehash LANGUAGES C CXX)

option(LIFEHASH_ENABLE_SANITIZERS "Build tests with ASan and UBSan" OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BC_LIFEHASH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/bc-lifehash/src")
set(BC_LIFEHASH_SOURCES
    bit-aggregator.cpp
    bit-enumerator.cpp
    cell-grid.cpp
    color.cpp
    color-func.cpp
    color-grid.cpp
    format-utils.cpp
    gradients.cpp
    hex.cpp
    hsb-color.cpp
    lifehash.cpp
    memzero.cpp
    patterns.cpp
    point.cpp
    sha256.cpp)
list(TRANSFORM BC_LIFEHASH_SOURCES PREPEND "${BC_LIFEHASH_DIR}/")

# Pinned upstream hex.cpp/lifehash.cpp rely on transitive standard includes that
# GNU libstdc++ does not provide. Keep vendored bytes exact and supply the
# missing declarations to those translation units only.
set_source_files_properties("${BC_LIFEHASH_DIR}/hex.cpp"
    PROPERTIES COMPILE_OPTIONS "-include;stdexcept")
set_source_files_properties("${BC_LIFEHASH_DIR}/lifehash.cpp"
    PROPERTIES COMPILE_OPTIONS "-include;cstring")

add_library(entropylab_lifehash STATIC
    src/lifehash_fingerprint.cpp
    ${BC_LIFEHASH_SOURCES})
target_include_directories(entropylab_lifehash
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${BC_LIFEHASH_DIR}")
target_compile_options(entropylab_lifehash PRIVATE -Wall -Wextra -Wpedantic)

add_executable(lifehash_tests tests/test_lifehash.cpp)
target_include_directories(lifehash_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${BC_LIFEHASH_DIR}")
target_link_libraries(lifehash_tests PRIVATE entropylab_lifehash)
target_compile_options(lifehash_tests PRIVATE -Wall -Wextra -Wpedantic)

if(LIFEHASH_ENABLE_SANITIZERS)
    target_compile_options(entropylab_lifehash PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(lifehash_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(lifehash_tests PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME lifehash_contract COMMAND lifehash_tests)
set_tests_properties(lifehash_contract PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
