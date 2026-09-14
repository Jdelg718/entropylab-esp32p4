#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${IDF_PATH:?Set IDF_PATH to an ESP-IDF v5.5.5 checkout and source its export.sh}"
[[ "$(git -C "$IDF_PATH" rev-parse HEAD)" == b774170ff46c393eeb5e495ea37936038d3f4f4f ]] || { printf '%s\n' 'Wrong ESP-IDF revision' >&2; exit 1; }
command -v idf.py >/dev/null
command -v riscv32-esp-elf-gcc >/dev/null
CARGO="${CARGO:-cargo}"
export CC_riscv32imafc_esp_espidf=riscv32-esp-elf-gcc
export AR_riscv32imafc_esp_espidf=riscv32-esp-elf-ar
export CFLAGS_riscv32imafc_esp_espidf='-march=rv32imafc -mabi=ilp32f -fno-pic -fno-pie'
export CARGO_TARGET_RISCV32IMAFC_ESP_ESPIDF_RUSTFLAGS='-C relocation-model=static'
"$CARGO" +nightly-2026-04-15 build --release --target riscv32imafc-esp-espidf -Zbuild-std=core,alloc --locked --manifest-path "$ROOT/fixture-firmware/rust/Cargo.toml" --target-dir "$ROOT/fixture-firmware/rust/target"
APP="$ROOT/fixture-firmware/app"
# Reproduce reviewed Rev1.3 configuration, not a stale developer sdkconfig.
cp "$APP/sdkconfig.baseline" "$APP/sdkconfig"
LOCK_SHA="$(sha256sum "$APP/dependencies.lock")"
cd "$APP"
idf.py -B "$ROOT/fixture-firmware/build" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' build
mkdir -p "$ROOT/fixture-firmware/logs"
riscv32-esp-elf-readelf -h "$ROOT/fixture-firmware/rust/target/riscv32imafc-esp-espidf/release/libentropylab_hex_core.a" > "$ROOT/fixture-firmware/logs/archive-headers.txt"
riscv32-esp-elf-nm "$ROOT/fixture-firmware/build/entropylab_fixture.elf" > "$ROOT/fixture-firmware/logs/symbols.txt"
python3 "$ROOT/fixture-firmware/verify.py"
# The build must not silently change the reviewed component resolution.
[[ "$(sha256sum "$APP/dependencies.lock")" == "$LOCK_SHA" ]] || { printf '%s\n' 'Component lock drift' >&2; exit 1; }
