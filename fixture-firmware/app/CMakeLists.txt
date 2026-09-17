cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# IDF 5.5.5 initializes CMAKE_*_FLAGS before CMake reads environment flags.
# Forward explicit initial flags to the independently configured bootloader.
if(DEFINED ENV{ENTROPYLAB_C_PREFIX_FLAGS})
    if("$ENV{ENTROPYLAB_C_PREFIX_FLAGS}" STREQUAL "")
        message(FATAL_ERROR "Empty release prefix maps")
    endif()
    foreach(lang C CXX ASM)
        idf_build_set_property(EXTRA_CMAKE_ARGS
            "-DCMAKE_${lang}_FLAGS=$ENV{ENTROPYLAB_C_PREFIX_FLAGS}" APPEND)
    endforeach()
endif()
project(entropylab_fixture)
if(DEFINED ENV{ENTROPYLAB_C_PREFIX_FLAGS})
    # Append to the real toolchain response files, preserving ABI/config flags.
    # This also covers generated project_elf_src, outside component targets.
    idf_toolchain_add_flags(COMPILE_OPTIONS "$ENV{ENTROPYLAB_C_PREFIX_FLAGS}")
    # Expose configuration alone so the release launcher can gate its graph.
    ExternalProject_Add_StepTargets(bootloader configure)
endif()
