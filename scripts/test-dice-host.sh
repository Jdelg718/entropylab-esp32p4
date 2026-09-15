#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CARGO="${CARGO:-cargo}"
python3 "$ROOT/scripts/test-dice-lock.py"
python3 "$ROOT/scripts/verify-dice-import.py"
python3 "$ROOT/scripts/test-dice-import.py"
for profile in debug release; do
 flags=(); [[ "$profile" != release ]] || flags+=(--release)
 "$CARGO" +"${HOST_TOOLCHAIN:-1.95.0}" test --locked --manifest-path "$ROOT/fixture-firmware/dice/Cargo.toml" "${flags[@]}"
done
"$CARGO" +"${HOST_TOOLCHAIN:-1.95.0}" build --release --locked --manifest-path "$ROOT/fixture-firmware/runtime/Cargo.toml" --target-dir "$ROOT/fixture-firmware/runtime/target"
mkdir -p "$ROOT/fixture-firmware/build"
"${CC:-cc}" -Wall -Wextra -Werror -UNDEBUG -I"$ROOT/fixture-firmware/app/main" "$ROOT/fixture-firmware/tests/dice_worker.c" "$ROOT/fixture-firmware/runtime/target/release/libentropylab_runtime.a" -ldl -lpthread -lm -o "$ROOT/fixture-firmware/build/dice_worker"
"$ROOT/fixture-firmware/build/dice_worker"
