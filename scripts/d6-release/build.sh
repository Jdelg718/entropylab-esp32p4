#!/usr/bin/env bash
set -euo pipefail
R="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
: "${IDF_PATH:?Set IDF_PATH to ESP-IDF v5.5.5 checkout}"
source "$IDF_PATH/export.sh"
[[ "$(git -C "$IDF_PATH" rev-parse HEAD)" == b774170ff46c393eeb5e495ea37936038d3f4f4f ]]
export PYTHONDONTWRITEBYTECODE=1 PYTHONOPTIMIZE=0
CARGO="${CARGO:-cargo}"
python3 "$R/scripts/d6-release/verify-source.py" --source "$R"
eval "$(python3 "$R/scripts/d6-release/exports.py")"
unset RUSTFLAGS CARGO_ENCODED_RUSTFLAGS
export CC_riscv32imafc_esp_espidf=riscv32-esp-elf-gcc AR_riscv32imafc_esp_espidf=riscv32-esp-elf-ar
B="$R/fixture-firmware/build"
printf 'Source identity: %s; recipe d6-r1; descriptor: %s\n' "$ENTROPYLAB_SOURCE_SHA256" "$ENTROPYLAB_PROJECT_VER"
rustc +nightly-2026-04-15 -Vv
riscv32-esp-elf-gcc --version
idf.py --version
cp "$R/fixture-firmware/app/sdkconfig.baseline" "$R/fixture-firmware/app/sdkconfig"
cd "$R/fixture-firmware/app"
LOCK_SHA="$(sha256sum dependencies.lock)"
mkdir -p "$R/fixture-firmware/logs"
idf.py -B "$B" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" reconfigure
python3 "$R/scripts/check-idf-remaps.py" "$B" app
cmake --build "$B" --target bootloader-configure
python3 "$R/scripts/check-idf-remaps.py" "$B/bootloader" bootloader
[[ "$(sha256sum dependencies.lock)" == "$LOCK_SHA" ]]
"$CARGO" +nightly-2026-04-15 build --release --target riscv32imafc-esp-espidf -Zbuild-std=core,alloc --locked --manifest-path "$R/fixture-firmware/runtime/Cargo.toml" --target-dir "$R/fixture-firmware/runtime/target"
idf.py -B "$B" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" build
riscv32-esp-elf-readelf -h "$R/fixture-firmware/runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a" > "$R/fixture-firmware/logs/archive-headers.txt"
riscv32-esp-elf-nm "$B/entropylab_fixture.elf" > "$R/fixture-firmware/logs/symbols.txt"
python3 "$R/fixture-firmware/verify.py"
python3 "$R/scripts/d6-release/verify-target.py" --source "$R" --build "$B" --archive "$R/fixture-firmware/runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a"
python3 "$R/scripts/test-runtime-lock.py"
[[ "$(sha256sum dependencies.lock)" == "$LOCK_SHA" ]]
