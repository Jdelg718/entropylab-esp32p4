#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LVGL_SOURCE_DIR="${LVGL_SOURCE_DIR:-$ROOT/fixture-firmware/app/managed_components/lvgl__lvgl}"
[[ -f "$LVGL_SOURCE_DIR/lvgl.h" ]] || { printf '%s\n' 'Set LVGL_SOURCE_DIR to LVGL 9.5.0 sources, or build firmware first.' >&2; exit 1; }
"${CARGO:-cargo}" +"${HOST_TOOLCHAIN:-1.95.0}" build --release --locked --manifest-path "$ROOT/fixture-firmware/rust/Cargo.toml" --target-dir "$ROOT/fixture-firmware/rust/target"
bash "$ROOT/scripts/test-coin-host.sh"
BUILD="$ROOT/fixture-firmware/build/gui-host"
cmake -S "$ROOT/fixture-firmware/tests" -B "$BUILD" -DLVGL_SOURCE_DIR="$LVGL_SOURCE_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --parallel "${BUILD_JOBS:-4}"
mkdir -p "$ROOT/fixture-firmware/logs/gui-host"
cd "$ROOT/fixture-firmware/logs/gui-host"
"$BUILD/gui_host"
