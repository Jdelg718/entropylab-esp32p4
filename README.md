# EntropyLab for ESP32-P4

**Unofficial experimental Bitcoin derivation calculator — PUBLIC-FIXTURE milestone only.**
Never enter real secrets or fund fixture addresses. This is not a wallet, hardware
signer, audited cryptographic product, or complete port of upstream EntropyLab.

## What actually works

A Rust `no_std` + `alloc` static library computes the standard BIP39 mnemonic
from 16 compiled zero bytes, the BIP32 master fingerprint, and the BIP84 mainnet
address at `m/84'/0'/0'/0/0`. A C ABI supplies bounded caller-owned buffers.
An internal-RAM FreeRTOS worker computes outside LVGL callbacks, and an LVGL
screen displays and compares the result. A touch button requests the calculation.
No private-input UI, signing, persistence, networking initialization or application
RNG is implemented. Volatile seed wiping does **not** guarantee erasure of library
temporaries, keys or allocations. OOM/panic aborts rather than unwinding across C.

The source fixture was flashed and passed on a Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3,
chip Rev1.3, with 32 MB PSRAM at 200 MHz and working display/touch. The recorded
calculation returned `rc=0 pass=1`. See [sanitized evidence](docs/HARDWARE-EVIDENCE.md).
This is evidence for the source milestone, not a claim that this reorganized
checkout has been freshly target-built or flashed. Host checks run independently.

## Quick start (Linux host)

Install Rust through rustup using the official Rust installation instructions,
a C compiler, Python 3, and git. Then from a checkout:

```sh
rustup toolchain install 1.95.0 --profile minimal
bash scripts/test-host.sh
```

Host tests exercise the separate bounded upstream extraction (`core-spike/native`),
public BIP39/BIP32/BIP84 vectors, malformed input, CLI guards, fixture ABI bounds,
and real C-to-Rust static linkage. Host tests do not prove embedded startup.
The host CLI accepts only its documented public-test interface: never supply secrets.

See [BUILD](docs/BUILD.md) for pinned target setup, [RECOVERY](docs/RECOVERY.md)
before flashing, [ROADMAP](ROADMAP.md), and [CONTRIBUTING](CONTRIBUTING.md).
GUI redesign is a future integration; no browser mockup is presented as firmware.

## Upstream and licensing

Selective reuse, not a full fork: [OogaBoogaX/entropylab](https://github.com/OogaBoogaX/entropylab)
commit `6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51`. The host extraction retains
its exact custom Ooga Booga license; new wrappers are MIT. The target fixture is
a direct library adapter, **not** the 15-function host extraction ported to no_std.
See [provenance](docs/PROVENANCE.md), LICENSE and THIRD_PARTY_NOTICES.md.

This limited source-fixture release passed independent public-content and license
review plus local host tests. A clean target build of this reorganized checkout
remains unverified. No upstream endorsement or contributor affiliation is implied.
