#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${IDF_PATH:?Set IDF_PATH to an ESP-IDF v5.5.5 checkout and source its export.sh}"
[[ "$(git -C "$IDF_PATH" rev-parse HEAD)" == b774170ff46c393eeb5e495ea37936038d3f4f4f ]] || { printf '%s\n' 'Wrong ESP-IDF revision' >&2; exit 1; }
command -v idf.py >/dev/null
command -v riscv32-esp-elf-gcc >/dev/null
CARGO="${CARGO:-cargo}"
python3 "$ROOT/scripts/test-runtime-lock.py"
export CC_riscv32imafc_esp_espidf=riscv32-esp-elf-gcc
export AR_riscv32imafc_esp_espidf=riscv32-esp-elf-ar
# Validate a fresh manifest-bound tree before generating any build outputs.
# Explicit IDF response-file injection and nested CMake flags; not CFLAGS inheritance.
HYGIENE_EXPORTS="$(python3 "$ROOT/scripts/release-hygiene.py")"
eval "$HYGIENE_EXPORTS"
unset RUSTFLAGS CARGO_ENCODED_RUSTFLAGS
printf 'Source manifest SHA256: %s; recipe: h2; descriptor: %s\n' "$ENTROPYLAB_SOURCE_SHA256" "$ENTROPYLAB_PROJECT_VER"
APP="$ROOT/fixture-firmware/app"
# Reproduce reviewed Rev1.3 configuration, not a stale developer sdkconfig.
cp "$APP/sdkconfig.baseline" "$APP/sdkconfig"
LOCK_SHA="$(sha256sum "$APP/dependencies.lock")"
cd "$APP"
BUILD="$ROOT/fixture-firmware/build"
mkdir -p "$ROOT/fixture-firmware/logs"
idf.py -B "$BUILD" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" reconfigure
python3 "$ROOT/scripts/check-idf-remaps.py" "$BUILD" app > "$ROOT/fixture-firmware/logs/app-remaps.json"
# Only the nested configure step; no app or bootloader target compilation.
cmake --build "$BUILD" --target bootloader-configure
python3 "$ROOT/scripts/check-idf-remaps.py" "$BUILD/bootloader" bootloader > "$ROOT/fixture-firmware/logs/bootloader-remaps.json"
[[ "$(sha256sum "$APP/dependencies.lock")" == "$LOCK_SHA" ]] || { printf '%s\n' 'Component lock drift during configure' >&2; exit 1; }
# Both actual graphs must pass before the expensive Rust/IDF builds.
"$CARGO" +nightly-2026-04-15 build --release --target riscv32imafc-esp-espidf -Zbuild-std=core,alloc --locked --manifest-path "$ROOT/fixture-firmware/runtime/Cargo.toml" --target-dir "$ROOT/fixture-firmware/runtime/target"
idf.py -B "$BUILD" -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;rev1_3.defaults' -D "PROJECT_VER=$ENTROPYLAB_PROJECT_VER" build
riscv32-esp-elf-readelf -h "$ROOT/fixture-firmware/runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a" > "$ROOT/fixture-firmware/logs/archive-headers.txt"
riscv32-esp-elf-nm "$ROOT/fixture-firmware/build/entropylab_fixture.elf" > "$ROOT/fixture-firmware/logs/symbols.txt"
python3 "$ROOT/fixture-firmware/verify.py"
python3 "$ROOT/scripts/verify-runtime-elf.py"
# The build must not silently change the reviewed component resolution.
[[ "$(sha256sum "$APP/dependencies.lock")" == "$LOCK_SHA" ]] || { printf '%s\n' 'Component lock drift' >&2; exit 1; }
