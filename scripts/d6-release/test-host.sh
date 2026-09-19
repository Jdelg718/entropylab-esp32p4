#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
: "${D6_HISTORICAL_ROOT:?Run via scripts/d6-release/test-host.py}"
CARGO="${CARGO:-cargo}"
TOOLCHAIN="${HOST_TOOLCHAIN:-1.95.0}"
python3 "$ROOT/scripts/d6-release/verify-source.py" --source "$ROOT"
python3 "${D6_HISTORICAL_ROOT}/scripts/test-d6-release-import.py"
python3 "$ROOT/scripts/d6-release/verify-source.py" --source "$ROOT"
python3 "${D6_HISTORICAL_ROOT}/scripts/test-review-repair-import.py"
python3 "$ROOT/scripts/d6-release/verify-source.py" --source "$ROOT"
python3 "${D6_HISTORICAL_ROOT}/scripts/test-modal-import.py"
python3 "$ROOT/scripts/d6-release/verify-source.py" --source "$ROOT"
python3 "${D6_HISTORICAL_ROOT}/scripts/test-global-saver-import.py"
python3 "$ROOT/scripts/test-coin-archive.py"
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
"${CC:-cc}" -I"$ROOT/fixture-firmware/rust" -Wall -Wextra -Werror "$ROOT/fixture-firmware/host-test.c" "$ROOT/fixture-firmware/rust/target/release/libentropylab_hex_core.a" -ldl -lpthread -lm -o "$ROOT/fixture-firmware/build/host-test"
"$ROOT/fixture-firmware/build/host-test" 00000000000000000000000000000000
python3 "$ROOT/scripts/test-hex-integration.py"

bash "$ROOT/scripts/test-coin-host.sh"
bash "$ROOT/scripts/d6-release/test-dice-host.sh"
bash "$ROOT/scripts/test-words-host.sh"
bash "$ROOT/scripts/test-gui-host.sh"
bash "$ROOT/scripts/test-combined-host.sh"
