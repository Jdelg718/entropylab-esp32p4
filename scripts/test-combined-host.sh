#!/usr/bin/env bash
# Public fixtures only. Runs real combined application and LifeHash sources.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
F="$ROOT/fixture-firmware"
LVGL_SOURCE_DIR="${LVGL_SOURCE_DIR:-$F/app/managed_components/lvgl__lvgl}"
"${CARGO:-cargo}" +"${HOST_TOOLCHAIN:-1.95.0}" build --release --locked --manifest-path "$F/runtime/Cargo.toml" --target-dir "$F/runtime/target"
bash "$F/integrated-lanes/lifehash/core/run-tests.sh"
for pair in 'integration37 lifehash_application_test' 'integration41 extra_dice_application_test' 'integration41-native extra_dice_native_test' 'integration22 application_gui_test' 'gui08-native gui08_native_tests'; do
  read -r suite executable <<< "$pair"
  build="$F/build/combined-$suite"
  cmake -S "$F/tests/$suite" -B "$build" -DLVGL_SOURCE_DIR="$LVGL_SOURCE_DIR" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$build" --parallel "${BUILD_JOBS:-4}"
  (cd "$build" && "./$executable")
done
