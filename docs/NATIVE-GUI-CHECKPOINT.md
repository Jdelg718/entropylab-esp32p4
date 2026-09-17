> Historical predecessor evidence only. Not current D6 build, installed identity or publication approval. See [current status](PUBLICATION-STATUS.md).

# Native GUI integration checkpoint

Local feature branch `native-gui`, based on public main
`4aea45ee852e5ce4d097af20b66ea9cdd18710ea`. Not yet independently reviewed,
published or flashed from this checkout.

## Scope and source identity

The reviewed source milestone main.c, gui.c, gui.h and five generated font C
files were imported byte-for-byte. GUI SHA-256:
`2b522b137d353a61dbb35325ac56164d4266c74e632009c5b8aaa8605a4c84c2`.
Thus palette, layout, worker integration and state behavior are unchanged.
No Rust core, Cargo lock, component lock or baseline configuration changes.
The user confirmed the source GUI's Run public fixture on hardware; this does
not establish runtime success of a new checkout binary.

Hex keypad, Clear and Delete remain disabled. No input buffer, arbitrary hex
API, signing, persistence or networking activation was introduced.

## Verified here

- `scripts/test-host.sh`: PASS with explicit external Cargo/rustup caches,
  `HOST_TOOLCHAIN=nightly-2026-04-15`, `CARGO_NET_OFFLINE=true`.
  Each debug/release profile passed 12 host-extraction tests and 2 fixture tests;
  real C/Rust ABI known-answer and capacity sentinel passed. Python drift suite:
  6 tests run, 1 skipped. This run used nightly, not the documented stable default.
- `scripts/test-gui-host.sh`: PASS with explicit `LVGL_SOURCE_DIR` pointing to
  existing LVGL 9.5.0 source. Fresh CMake host objects in this checkout;
  navigation, disabled keypad, busy/retry/failure, label bounds and minimum
  44px targets: 51 controls checked across states. Generated PPMs stay ignored.
- Five font payloads regenerated using lv_font_conv 1.5.3 and exactly matched
  after the generated invocation comment. See font provenance and full OFL.
- `git diff --check`: PASS; tracked and proposed source scan found no private
  absolute workspace paths or symlinks. This is not independent security review.

## Target build PASS (2026-09-14 UTC)

The existing portable `scripts/build-firmware.sh` was actually launched with
external `CARGO`, `CARGO_HOME`, `RUSTUP_HOME`, `IDF_TOOLS_PATH`, and sourced
ESP-IDF v5.5.5 export environment. Cargo offline cache reuse was explicit.
Before launch, no target Rust directory and no target ELF existed in this checkout.
It compiled core/alloc and the target Rust dependencies afresh, then materialized
locked managed components locally and entered a fresh 2059-step IDF build.
No source dependency symlink or old prelinked ELF/archive was imported.

The original process completed all 2059 IDF steps and exited **0**, within its
900-second bound. Its live process was checked before continuing; no competing
build or retry was started. The build's automatic verifier passed, and after
inspecting `fixture-firmware/verify.py`, a separate invocation also exited **0**.
The report contains `status: PASS`, 83 archive members with single-float ABI,
revision bounds `[100, 199]`, and `runtime_hardware_tested: false`. Required
`fixture_run`, `fixture_alloc`, and `fixture_free` symbols passed inspection.
The build's final component-lock drift check passed.

The generated sdkconfig was read directly: minimum revision 100, maximum 199,
PSRAM speed 200 MHz, flash size 32 MB. Both application and bootloader image
headers independently decoded to revision bounds `(100, 199)`. The verifier's
source checks are a limited forbidden-activation scan, not a security audit.
The JSON report is in ignored `fixture-firmware/logs/verification.json`.

### Completed artifact evidence

Paths below are relative to `fixture-firmware/`; SHA-256 values were computed
from completed files after process exit, not inferred from build messages.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `build/entropylab_fixture.bin` | 1017744 | `f76255c7af216c413e677eb441582826cf8a18a6c62e7e73cb84d76040382802` |
| `build/entropylab_fixture.elf` | 13766196 | `d8c7448734ffbf111a0913d1e19261157cf941f04af20da1e19a492f3fd68d9f` |
| `build/bootloader/bootloader.bin` | 21264 | `b53618986183877411255f0d27c1316659fb4ca94e4fb8174766db3e45d978e4` |
| `build/partition_table/partition-table.bin` | 3072 | `d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb` |

A fresh dependency download/toolchain-install test and relocated-checkout build
are not claimed. Scripts resolve their checkout and accept explicit cache roots;
no private workspace path is embedded in distributable sources.

## Source-freeze handoff for independent SPEC/security review

No feature or source edits were made during target-evidence finalization. The
eight imported main/GUI/header/font source files were compared again with the
reviewed milestone and remain byte-identical. Host test results above are the
prior worker's recorded executions; target completion and the separate verifier
were observed during finalization. Rust source, Cargo locks, component lock and
baseline configuration remain unchanged against the stated base.

Complete proposed diff, including untracked files (20 files):

```text
README.md
THIRD_PARTY_NOTICES.md
docs/NATIVE-GUI-CHECKPOINT.md
fixture-firmware/app/main/CMakeLists.txt
fixture-firmware/app/main/fonts/LICENSE-Liberation.txt
fixture-firmware/app/main/fonts/LICENSE-OFL-1.1
fixture-firmware/app/main/fonts/PROVENANCE.md
fixture-firmware/app/main/fonts/el_mono_16.c
fixture-firmware/app/main/fonts/el_mono_20.c
fixture-firmware/app/main/fonts/el_sans_14.c
fixture-firmware/app/main/fonts/el_sans_16.c
fixture-firmware/app/main/fonts/el_serif_24.c
fixture-firmware/app/main/gui.c
fixture-firmware/app/main/gui.h
fixture-firmware/app/main/main.c
fixture-firmware/tests/CMakeLists.txt
fixture-firmware/tests/gui_host.c
fixture-firmware/tests/lv_conf.h
fixture-firmware/verify.py
scripts/test-gui-host.sh
```

Independent SPEC/security review remains required before commit publication,
merge or flashing. This is a source-freeze handoff, not independent approval.
Nothing was committed, pushed or flashed. A target build and header/config
checks are not runtime, display, touch, memory-stability or hardware proof for
this new binary; the earlier hardware confirmation concerns the source milestone.
