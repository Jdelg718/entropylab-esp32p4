#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CARGO="${CARGO:-cargo}"
for profile in debug release; do
 flags=(); [[ "$profile" != release ]] || flags+=(--release)
 "$CARGO" +1.95.0 test --locked --manifest-path "$ROOT/fixture-firmware/coin/Cargo.toml" "${flags[@]}"
done
python3 "$ROOT/fixture-firmware/coin/verify-fixtures.py"
for core in runtime; do
 "$CARGO" +1.95.0 build --release --locked --manifest-path "$ROOT/fixture-firmware/$core/Cargo.toml" --target-dir "$ROOT/fixture-firmware/$core/target"
done
mkdir -p "$ROOT/fixture-firmware/build"
"${CC:-cc}" -Wall -Wextra -Werror -UNDEBUG -I"$ROOT/fixture-firmware/app/main" "$ROOT/fixture-firmware/tests/worker_host.c" "$ROOT/fixture-firmware/runtime/target/release/libentropylab_runtime.a" -ldl -lpthread -lm -o "$ROOT/fixture-firmware/build/worker_host"
"$ROOT/fixture-firmware/build/worker_host" "$ROOT/fixture-firmware/coin/vectors/raw-binary.tsv"
