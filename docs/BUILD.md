# Rebuilding

Host: `bash scripts/test-host.sh` (Rust 1.95.0, Python 3, Linux C toolchain).
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
Rust relocation is static. IDF's C linker performs final linkage of the Rust archive.
Do not substitute the integer-only RISC-V ABI or default upstream Rev3 config.
BSP 1.0.1 and LVGL 9.5.0, plus transitive component hashes, are fixed in
fixture-firmware/app/dependencies.lock. CMake suppresses two known LVGL warning
classes only; it does not globally disable warnings-as-errors.

The post-build verifier checks app and bootloader revision range 100..199,
200 MHz PSRAM configuration, Rust archive single-float ABI and linked C ABI symbols.
It does not prove timing, UI layout, RAM headroom or runtime success. No automatic
flashing is provided. Hardware recovery/authorization comes first.

The portable target script/configuration has been inspected, but a complete clean
target build of this layout is not claimed until separately run and recorded.
