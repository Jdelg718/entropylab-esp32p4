cmake_minimum_required(VERSION 3.16)
project(application22_focused C CXX)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../lv_conf.h" CACHE PATH "" FORCE)
set(LVGL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../app/managed_components/lvgl__lvgl" CACHE PATH "Pinned LVGL source; override with -DLVGL_SOURCE_DIR")
add_subdirectory("${LVGL_SOURCE_DIR}" lvgl)
add_subdirectory(../../integrated-lanes/lifehash/core lifehash-core)
add_executable(application_gui_test
    application_gui_test.cpp
    ../../app/main/all_features_application.cpp
    ../../app/main/all_features_dispatch.cpp
    ../../app/main/gui08_native.c
    ../../app/main/fonts/el_sans_16.c
    ../../app/main/fingerprint_view.c
    ../../app/main/gui08_editor.c
    ../../integrated-lanes/bases/core/number_bases.c)
target_include_directories(application_gui_test PRIVATE
    ../../app/main ../../integrated-lanes/bases/core)
target_include_directories(application_gui_test PRIVATE
    ../../integrated-lanes/lifehash/core/include)
target_link_libraries(application_gui_test PRIVATE lvgl entropylab_lifehash m pthread)
target_compile_options(application_gui_test PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=c11;-Wall;-Wextra;-Werror;-UNDEBUG>
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++17;-Wall;-Wextra;-Werror;-UNDEBUG>)
