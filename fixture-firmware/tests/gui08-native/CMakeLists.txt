cmake_minimum_required(VERSION 3.16)
project(gui08_native_focused C)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../lv_conf.h" CACHE PATH "" FORCE)
set(LVGL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../app/managed_components/lvgl__lvgl" CACHE PATH "Pinned LVGL source; override with -DLVGL_SOURCE_DIR")
add_subdirectory("${LVGL_SOURCE_DIR}" lvgl)
add_executable(gui08_native_tests
    ../gui08_native_tests.c
    ../../app/main/gui08_native.c
    ../../app/main/fonts/el_sans_16.c
    ../../app/main/fingerprint_view.c
    ../../app/main/gui08_editor.c
    ../../integrated-lanes/bases/core/number_bases.c)
target_include_directories(gui08_native_tests PRIVATE ../../app/main ../../integrated-lanes/bases/core)
target_link_libraries(gui08_native_tests PRIVATE lvgl m pthread)
target_compile_options(gui08_native_tests PRIVATE -std=c11 -Wall -Wextra -Werror -UNDEBUG)
