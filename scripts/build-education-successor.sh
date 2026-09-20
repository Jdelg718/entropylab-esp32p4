#!/usr/bin/env bash
# Candidate-only build. Does not invoke any historical accepted-source exporter.
set -euo pipefail
: "${EDUCATION_SOURCE:?fresh staged candidate source required}"
: "${EDUCATION_EXPORTS:?verified candidate export file required}"
: "${IDF_PATH:?pinned ESP-IDF required}"
source "$IDF_PATH/export.sh"
[[ "$(git -C "$IDF_PATH" rev-parse HEAD)" == b774170ff46c393eeb5e495ea37936038d3f4f4f ]]
source "$EDUCATION_EXPORTS"
unset RUSTFLAGS CARGO_ENCODED_RUSTFLAGS
export CC_riscv32imafc_esp_espidf=riscv32-esp-elf-gcc AR_riscv32imafc_esp_espidf=riscv32-esp-elf-ar
R="$EDUCATION_SOURCE"; B="$R/fixture-firmware/build"
printf 'Education source identity %s; candidate descriptor %s\n' "$ENTROPYLAB_SOURCE_SHA256" "$ENTROPYLAB_PROJECT_VER"
rustc +nightly-2026-04-15 -Vv
riscv32-esp-elf-gcc --version
idf.py --version
cmp "$R/fixture-firmware/app/sdkconfig.baseline" "$R/fixture-firmware/app/sdkconfig"
cd "$R/fixture-firmware/app"
LOCK_SHA="$(sha256sum dependencies.lock)"
mkdir -p "$R/fixture-firmware/logs"
idf.py -B "$B" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" reconfigure
python3 "$R/scripts/check-idf-remaps.py" "$B" app
cmake --build "$B" --target bootloader-configure
python3 "$R/scripts/check-idf-remaps.py" "$B/bootloader" bootloader
[[ "$(sha256sum dependencies.lock)" == "$LOCK_SHA" ]]
"${CARGO:-cargo}" +nightly-2026-04-15 build --release --target riscv32imafc-esp-espidf -Zbuild-std=core,alloc --locked --manifest-path "$R/fixture-firmware/runtime/Cargo.toml" --target-dir "$R/fixture-firmware/runtime/target"
idf.py -B "$B" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" build
riscv32-esp-elf-readelf -h "$R/fixture-firmware/runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a" > "$R/fixture-firmware/logs/archive-headers.txt"
riscv32-esp-elf-nm "$B/entropylab_fixture.elf" > "$R/fixture-firmware/logs/symbols.txt"
python3 "$R/fixture-firmware/verify.py"
python3 "$R/scripts/verify-runtime-elf.py"
python3 "$R/scripts/test-runtime-lock.py"
[[ "$(sha256sum dependencies.lock)" == "$LOCK_SHA" ]]
