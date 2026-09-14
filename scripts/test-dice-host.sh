#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CARGO="${CARGO:-cargo}"
python3 "$ROOT/scripts/test-dice-lock.py"
python3 "$ROOT/scripts/verify-dice-import.py"
for profile in debug release; do
 flags=(); [[ "$profile" != release ]] || flags+=(--release)
 "$CARGO" +"${HOST_TOOLCHAIN:-1.95.0}" test --locked --manifest-path "$ROOT/fixture-firmware/dice/Cargo.toml" "${flags[@]}"
done
"$CARGO" +"${HOST_TOOLCHAIN:-1.95.0}" build --release --locked --manifest-path "$ROOT/fixture-firmware/dice/Cargo.toml" --target-dir "$ROOT/fixture-firmware/dice/target"
mkdir -p "$ROOT/fixture-firmware/build"
"${CC:-cc}" -Wall -Wextra -Werror -UNDEBUG -I"$ROOT/fixture-firmware/app/main" "$ROOT/fixture-firmware/tests/dice_worker.c" "$ROOT/fixture-firmware/coin/target/release/libentropylab_coin_core.a" "$ROOT/fixture-firmware/dice/target/release/libentropylab_dice_core.a" -ldl -lpthread -lm -o "$ROOT/fixture-firmware/build/dice_worker"
"$ROOT/fixture-firmware/build/dice_worker"
