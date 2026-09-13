#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CARGO="${CARGO:-cargo}"
TOOLCHAIN="${HOST_TOOLCHAIN:-1.95.0}"
python3 "$ROOT/core-spike/prepare_vectors.py"
python3 "$ROOT/core-spike/test_drift.py"
for profile in debug release; do
  flags=(); if [[ "$profile" == release ]]; then flags+=(--release); fi
  "$CARGO" +"$TOOLCHAIN" test --locked --manifest-path "$ROOT/core-spike/native/Cargo.toml" "${flags[@]}"
  "$CARGO" +"$TOOLCHAIN" test --locked --manifest-path "$ROOT/fixture-firmware/rust/Cargo.toml" "${flags[@]}"
done
# Explicit output directory prevents inherited CARGO_TARGET_DIR from breaking C linkage.
"$CARGO" +"$TOOLCHAIN" build --release --locked --manifest-path "$ROOT/fixture-firmware/rust/Cargo.toml" --target-dir "$ROOT/fixture-firmware/rust/target"
mkdir -p "$ROOT/fixture-firmware/build"
"${CC:-cc}" -Wall -Wextra -Werror "$ROOT/fixture-firmware/host-test.c" "$ROOT/fixture-firmware/rust/target/release/libentropylab_fixture.a" -ldl -lpthread -lm -o "$ROOT/fixture-firmware/build/host-test"
"$ROOT/fixture-firmware/build/host-test"
