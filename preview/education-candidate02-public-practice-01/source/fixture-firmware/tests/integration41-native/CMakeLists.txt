cmake_minimum_required(VERSION 3.16)
project(extra_dice_native_test C CXX)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../lv_conf.h" CACHE PATH "" FORCE)
set(LVGL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../app/managed_components/lvgl__lvgl" CACHE PATH "Pinned LVGL source; override with -DLVGL_SOURCE_DIR")
add_subdirectory("${LVGL_SOURCE_DIR}" lvgl)
add_subdirectory(../../integrated-lanes/lifehash/core lifehash-core)
add_executable(extra_dice_native_test
  extra_dice_native_test.cpp
  ../../app/main/all_features_application.cpp
  ../../app/main/all_features_dispatch.cpp
  ../../app/main/extra_dice_native.c
  ../../app/main/fonts/el_sans_16.c
  ../../app/main/fingerprint_view.c
  ../../app/main/gui08_editor.c
  ../../input-methods/extra-dice/extra_dice.c
  ../../integrated-lanes/extra-dice/target-adapter/extra_dice_target.c
  ../../integrated-lanes/seedhelpers/target-adapter/seed_helpers_target.c
  ../../integrated-lanes/bases/core/number_bases.c)
target_include_directories(extra_dice_native_test PRIVATE
  ../../app/main ../../input-methods/extra-dice
  ../../integrated-lanes/extra-dice/target-adapter
  ../../integrated-lanes/seedhelpers/target-adapter
  ../../integrated-lanes/playingcards/adapter/include
  ../../integrated-lanes/bases/core
  ../../integrated-lanes/lifehash/core/include)
target_link_libraries(extra_dice_native_test PRIVATE lvgl entropylab_lifehash
  "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/target/release/libentropylab_runtime.a" m dl pthread)
target_compile_options(extra_dice_native_test PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-std=c11;-Wall;-Wextra;-Werror;-pedantic;-UNDEBUG>
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++17;-Wall;-Wextra;-Werror;-pedantic;-UNDEBUG>)
