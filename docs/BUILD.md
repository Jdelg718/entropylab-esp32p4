# Rebuilding

## Host suite

Requirements: Rust 1.95.0 through rustup, Python 3, C/C++ compilers and CMake.
The full `bash scripts/test-host.sh` now includes Words and native GUI tests.
GUI tests require LVGL **9.5.0 source**, not only installed headers. Either run
component setup through the target build below, or provide an external source tree:

```sh
rustup toolchain install 1.95.0 --profile minimal
mkdir -p "$HOME/.cache/entropylab"
git clone --depth 1 --branch v9.5.0 https://github.com/lvgl/lvgl.git "$HOME/.cache/entropylab/lvgl"
git -C "$HOME/.cache/entropylab/lvgl" checkout 85aa60d18b3d5e5588d7b247abf90198f07c8a63
export LVGL_SOURCE_DIR="$HOME/.cache/entropylab/lvgl"
bash scripts/test-host.sh
```

The default source location is `fixture-firmware/app/managed_components/lvgl__lvgl`.
`BUILD_JOBS` controls GUI build parallelism. Host GUI output is written beneath
`fixture-firmware/logs/gui-host`. No simulator rendering is physical-device evidence.
Words and GUI tests freshly compile `fixture-firmware/runtime`; older standalone
HEX/Coins/Dice suites remain regression checks, not the target link graph.

All Cargo invocations use `--locked`. Standard `CARGO_HOME` and `RUSTUP_HOME`
can point to external caches; `CARGO` may name a rustup cargo proxy. Offline reuse
is opt-in with `CARGO_NET_OFFLINE=true`; fresh users need network dependency access.
No toolchain, registry, managed component or upstream tree is bundled.

## Target setup from scratch

Install ESP-IDF Linux prerequisites per its official v5.5 documentation (CMake,
Ninja, Python venv support, git, compiler prerequisites), then:

```sh
mkdir -p "$HOME/.cache/entropylab"
git clone --recursive --branch v5.5.5 https://github.com/espressif/esp-idf.git "$HOME/.cache/entropylab/esp-idf"
git -C "$HOME/.cache/entropylab/esp-idf" checkout b774170ff46c393eeb5e495ea37936038d3f4f4f
git -C "$HOME/.cache/entropylab/esp-idf" submodule update --init --recursive
export IDF_TOOLS_PATH="$HOME/.cache/entropylab/idf-tools"
"$HOME/.cache/entropylab/esp-idf/install.sh" esp32p4
. "$HOME/.cache/entropylab/esp-idf/export.sh"
rustup toolchain install nightly-2026-04-15 --profile minimal --component rust-src
bash scripts/build-firmware.sh
```

Run the final script from anywhere; it resolves its checkout location. IDF's export
sets IDF_PATH. Run with a clean shell if inherited Python virtualenv variables
conflict with IDF. The script requires the exact IDF commit, restores the reviewed
sdkconfig.baseline, and refuses component-lock drift. Discard intentional config
experiments or update baseline only in a separately reviewed change.

Rust target: `riscv32imafc-esp-espidf`; nightly builds core and alloc using
`-Zbuild-std=core,alloc`. C dependencies use `-march=rv32imafc -mabi=ilp32f`.
Rust relocation is static. The script compiles `fixture-firmware/runtime` from the
copied cores in `runtime-sources` and IDF's C linker links the freshly produced
`libentropylab_runtime.a`. No old prebuilt runtime archive is distributed or linked.
The unified runtime Cargo.lock governs this build; nested historical standalone
locks are not its effective dependency graph. `scripts/test-runtime-lock.py`
checks the unified graph and mismatch cases.
Do not substitute the integer-only RISC-V ABI or default upstream Rev3 config.
BSP 1.0.1 and LVGL 9.5.0, plus transitive component hashes, are fixed in
fixture-firmware/app/dependencies.lock. CMake suppresses two known LVGL warning
classes only; it does not globally disable warnings-as-errors.

The post-build verifier checks app and bootloader revision range 100..199,
200 MHz PSRAM configuration, Rust archive single-float ABI and linked C ABI symbols.
`scripts/verify-runtime-elf.py` additionally checks the unified runtime/API symbols
and ABI, not historical installed-image identity. These checks do not prove timing,
UI layout, RAM headroom or runtime success. No automatic
flashing is provided. Hardware recovery/authorization comes first.

## Current candidate verification boundary

The complete `bash scripts/test-host.sh` gate passed (inner exit 0) on a fresh source-only copy of combined manifest `0c11e6fd9fcccc260f78918367c588c5095c755382cad35920a5c88a3ff7de10`. It compiled fresh host artifacts, ran the default GUI and all ten pointer-family slices, and continued through the combined native suite. Rust 1.95.0, GCC 14.2.0 and CMake 3.31.6 were inspected; existing LVGL sources and an isolated copy of the dependency registry were reused. This is clean-source/build-directory host assurance, not a fresh-network dependency test, ESP32-P4 build, binary reproducibility certification or hardware acceptance. Upstream dependency compiler warnings were retained, not represented as warning-free success.

The same combined source received bounded independent source review: 37 focused tests and 74 independent semantic probes (three accepted valid controls, 71 rejected invalid mutations). All original findings are closed for that reviewed code. This package changes documentation/status only after those gates; `HOST-VERIFICATION.json` binds the tested manifest and stream hashes. Production, tests, scripts, lockfiles and historical successor manifests remain byte-identical to the tested package.

Complete target image/ELF/map/resource/layout verification, exact pushed-SHA CI, first-install/update/recovery tests, physical acceptance and release/flasher review remain separate pending gates. No release, download URL, installed identity or publication approval is conveyed.
