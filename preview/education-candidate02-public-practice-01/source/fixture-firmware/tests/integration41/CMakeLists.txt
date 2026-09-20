cmake_minimum_required(VERSION 3.16)
project(extra_dice_application_test C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_executable(extra_dice_application_test
  extra_dice_application_test.cpp
  ../../app/main/all_features_dispatch.cpp
  ../../app/main/all_features_application.cpp
  ../../input-methods/extra-dice/extra_dice.c
  ../../integrated-lanes/extra-dice/target-adapter/extra_dice_target.c
  ../../integrated-lanes/seedhelpers/target-adapter/seed_helpers_target.c)
target_include_directories(extra_dice_application_test PRIVATE
  ../../app/main
  ../../input-methods/extra-dice
  ../../integrated-lanes/extra-dice/target-adapter
  ../../integrated-lanes/seedhelpers/target-adapter
  ../../integrated-lanes/playingcards/adapter/include)
target_compile_options(extra_dice_application_test PRIVATE -Wall -Wextra -Werror -pedantic -UNDEBUG)
target_link_libraries(extra_dice_application_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/target/release/libentropylab_runtime.a" dl pthread)
