#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
F="$ROOT/fixture-firmware"
CARGO="${CARGO:-cargo}"
TOOLCHAIN="${HOST_TOOLCHAIN:-1.95.0}"
python3 "$ROOT/scripts/test-runtime-lock.py"
for profile in debug release; do
 flags=(); [[ "$profile" != release ]] || flags+=(--release)
 "$CARGO" +"$TOOLCHAIN" test --locked --manifest-path "$F/runtime/Cargo.toml" "${flags[@]}"
done
"$CARGO" +"$TOOLCHAIN" build --release --locked --manifest-path "$F/runtime/Cargo.toml" --target-dir "$F/runtime/target"
mkdir -p "$F/build"
for test in mnemonic_bounds mnemonic_red mnemonic_bridge hex_worker worker_host dice_worker; do
 "${CC:-cc}" -Wall -Wextra -Werror -UNDEBUG -I"$F/app/main" "$F/tests/$test.c" "$F/runtime/target/release/libentropylab_runtime.a" -ldl -lpthread -lm -o "$F/build/$test"
 if [[ "$test" == worker_host ]]; then
   "$F/build/$test" "$F/coin/vectors/raw-binary.tsv"
 elif [[ "$test" != mnemonic_bridge ]]; then (cd "$F/rust" && "$F/build/$test"); fi
done
python3 "$ROOT/scripts/test-runtime-lock.py"
