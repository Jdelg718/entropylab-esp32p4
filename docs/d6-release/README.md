> **Current guidance:** [README — current retention preview](../current/docs/d6-release/README.md).
> The text below is the historical source checkpoint, not current release status.
> v0.1.0-retention-preview is published; v0.1.1 is a local documentation/onboarding patch pending publication.
> Firmware is unchanged; public-policy hardware qualification and the separate retention updater remain held.

# D6 reserved-heap candidate: portable successor recipe

The existing device-tested application predates this public source export. Its source identity is **07d03ba6f421a25316e5b73662f1a6f9d79243c7bdaba8119c3cd27a16573d4d** (SHA256 of the exact `candidate-source-identity.json` bytes). Application BIN SHA256: **1f487910cdb362a02e82d5836e017c6ebd62e99c1d2a3f4be68a04b84efcd3f4**. No prior source commit is invented. No byte-reproducibility claim is made.

Exactly 436 candidate paths at repository-relative locations remain byte-bound by that identity. All historical manifests, original build scripts and old allocator verifier remain untouched. Files added in `scripts/d6-release`, `docs/d6-release` and `docs/release-notices` are **successor packaging/recipe additions**, not inputs claimed to have built the existing artifact. The adjacent recipe-additions manifest identifies these separately. Whole-checkout equality to the historic snapshot is NOT claimed.

## Current entrypoints (replace historical h2 commands)

From the repository root, with Python 3:

```sh
python3 scripts/d6-release/verify-source.py
python3 scripts/d6-release/test-source.py
```

The validator verifies all 436 files, both pinned full manifests, the exact inventory and finite two-file delta from the combined candidate. It is a finite source identity check, not an independent re-audit of the historical hardware/evidence chain. Those historical records remain historical and are not rewritten into public build receipts.

## Full host tests

Install Rust 1.95.0 via rustup, a C/C++ compiler, CMake, Python 3, and native GUI dependencies described in `docs/BUILD.md`. Supply LVGL **9.5.0** sources matching the component lock. Cargo fetches exact locked registry versions unless `CARGO_NET_OFFLINE=true` is selected with an already populated cache.

```sh
export HOST_TOOLCHAIN=1.95.0
export LVGL_SOURCE_DIR=/your/dependencies/lvgl-9.5.0
# Optional: CARGO, RUSTUP_HOME, CARGO_HOME, BUILD_JOBS
python3 scripts/d6-release/test-host.py
```

The runner reconstructs only the three finite historical host-gate input differences in a temporary directory and runs the unchanged historical identity/negative tests there. Every executable Rust, C and GUI test uses the current candidate sources. The historical delta manifest and its source files are pinned and validated; this is not disabling old gates or pretending old gates accept new source. Host outputs go to ignored target/build/log directories. Never use the immutable existing target-build directory for host work.

## Inspect an existing target build without writing it

After sourcing the matching ESP-IDF environment (or setting `RISCV_TOOL_PREFIX`):

```sh
python3 scripts/d6-release/verify-target.py \
  --source . --build /your/immutable/fixture-firmware/build \
  --archive /your/immutable/fixture-firmware/runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a
```

Optional `--tool-prefix /your/toolchain/bin/riscv32-esp-elf-`. This retains ABI, linkage, panic and duplicate-symbol checks and checks the candidate's reserved 8192-byte DRAM pool, multi-heap allocation/free, locking and initialization order. It prints JSON to stdout only; no writes into the supplied build. It is static evidence, not a hardware or whole-app safety proof.

## Build a NEW candidate (not run for this release export)

Use a separate disposable clone, NOT the immutable existing build. Install ESP-IDF **v5.5.5**, exact commit **b774170ff46c393eeb5e495ea37936038d3f4f4f**, and its tools; Rust **nightly-2026-04-15** with rust-src; and Cargo. See the retained component and Rust locks. Set writable `CARGO_HOME` explicitly (e.g. `$HOME/.cargo`), `RUSTUP_HOME` if needed and `IDF_PATH` to that IDF checkout. Use paths without whitespace or shell/compiler-map metacharacters.

```sh
export IDF_PATH=/your/dependencies/esp-idf
export CARGO_HOME="$HOME/.cargo"
bash scripts/d6-release/build.sh
```

This successor recipe checks source identity, derives the existing `d6...-r1` descriptor from the full identity, enables the existing file-prefix privacy maps, retains lock checks and invokes the corrected allocator verifier. No private absolute paths are baked into this recipe. Managed components are fetched using the retained lock. This recipe has been shell-syntax checked but deliberately has NOT rebuilt firmware. A newly built BIN is a new artifact requiring its own verification and hardware qualification, even if its descriptor matches. Descriptor equality is not artifact equality.

## Distribution and notices

Use `docs/release-notices/manifest.json` and `CHECKLIST.md`. Preserve the custom Ooga Booga text, MIT and all applicable dependency notices. The bundle is bounded full-text collection, not a legal guarantee; unresolved checklist items block claiming complete binary notice closure. Only reviewed BINs and notices belong in a binary release; never ELF/MAP, raw build metadata, logs or private receipts.

The existing app is experimental/public-practice-test-only on the compatible Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 Rev1.x board with the proven installed layout/bootloader. App-only physical success does not qualify the new bootloader/partition tuple or generic first installation. Never use real wallet secrets; no erase/eFuse/C6 actions are authorized by this source export.
