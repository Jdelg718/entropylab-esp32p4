# EntropyLab for ESP32-P4

**Unofficial offline seed/mnemonic + fingerprint generator for supported
 general-purpose hardware — currently an experimental public-test HEX + COIN + DICE + WORDS candidate.**

The product direction is entropy-process assurance: traceable input handling,
explicit entropy-source assumptions and independently checked derivation, not a
transaction or signing tool. Public-test-only use reflects current readiness,
not a permanent product limitation; real-secret use still needs an explicit
threat-model and readiness review. Statistical tests cannot prove randomness or
physical entropy provenance. Offline application operation does not establish a
physical air gap; the board's C6 presence remains a hardware threat-model concern.
See the [roadmap and acceptance gates](ROADMAP.md): Words and practice cleanup are
a local candidate atop the published Dice checkpoint; broader upstream features remain deferred, and optional microSD seed export is
deferred behind a design gate (explicit user action only, never automatic storage;
no export implementation or SD writes now).

COIN accepts strict raw ASCII `0`/`1` transcripts: **Heads = 0, Tails = 1**,
MSB first, with exactly **128/160/192/224/256 bits** for **12/15/18/21/24 words**.
No whitespace, alternate alphabet or excess/short input is accepted by the adapter.
This is direct bit-to-hex conversion, **no hash conditioning and no entropy
assurance**. A BIP39 checksum does not make biased or predictable input random.
Never use real secrets or funds. Two public-zero presets were observed on the
reviewed source image, not on a new repository binary; manual flips, Undo and
mode isolation are host-tested only. See [COIN provenance and review gate](docs/COIN-MILESTONE.md).
Never enter real secrets or fund test addresses. This is not a wallet, hardware
signer, audited cryptographic product, or complete port of upstream EntropyLab.

DICE accepts ASCII `1`–`6`, up to 1024 rolls, for all five word counts.
Raw COLDCARD-style SHA256 and Coleman `6`→`0` before SHA256 are separate
transcripts. Short input remains visibly `WEAK_INPUT_LAB_ONLY`; meeting the
nominal count is not entropy assurance. See [DICE milestone](docs/DICE-MILESTONE.md)
for the two short public hardware observations and source identity limits.

## Words and practice cleanup

The candidate adds canonical English BIP39 Words validation at all five word
counts, prefix selection, a taller list, practical selected-mode Clear and common
Safety/About. The Words-only authentic Orbit saver starts after 60 seconds of eligible inactivity,
conceals and preserves committed input, and consumes the first wake contact.
It is not a lock/wipe. Current installed-source
physical acceptance remains pending; a new repository build is a distinct image.
See [Words milestone](docs/WORDS-MILESTONE.md) and [handoff](docs/HANDOFF.md).

## What actually works

The native 480×800 LVGL touchscreen accepts hexadecimal public test input with
0–F keys, Delete, Clear, and Calculate. Supported lengths are **32/40/48/56/64
hex characters** (128/160/192/224/256 bits), producing **12/15/18/21/24 BIP39
English words**. `Load public zero` loads **64 zero hex characters, 256 bits**,
not the older 128-bit fixture. The label is retained from the hardware-tested
source; the counter shows the exact length and bit width.

A Rust `no_std` + `alloc` library computes the mnemonic, BIP32 master fingerprint,
and **only the first mainnet BIP84 address** at `m/84'/0'/0'/0/0`, using the
**empty BIP39 passphrase only**. The HEX parser rejects whitespace rather than
trimming/normalizing it; this interface is deliberately narrower than upstream.
A bounded C ABI uses caller-owned output capacities of **216/9/43 bytes**
(mnemonic/fingerprint/address, including terminators). An internal-RAM FreeRTOS
worker receives owned queue copies and computes outside LVGL callbacks.

Editing invalidates prior results; busy-state controls prevent overlapping work.
Clear removes input and prior displayed results. It does **not** prove secure
erasure of stacks, queues, library temporaries, keys or allocations. Best-effort
wiping is not a secret-safe memory guarantee. OOM/panic aborts rather than
unwinding across C. There are no RNG, signing, storage, or networking application
paths. **Public test inputs only**, despite the editable interface.

The reviewed source HEX binary was exercised on hardware. See the exact
[sanitized manual record](docs/HEX-MILESTONE.md), which distinguishes user screen
observations from host comparison. That is not a flash/runtime claim for a new
binary built from this checkout; no formal independent cryptographic oracle or
secure-erasure verification is claimed. Previous fixture-only evidence remains
historical in [HARDWARE-EVIDENCE](docs/HARDWARE-EVIDENCE.md) and
[NATIVE-GUI-CHECKPOINT](docs/NATIVE-GUI-CHECKPOINT.md).

## Quick start (Linux host)

Install Rust through rustup, C/C++ compilers, CMake, Python 3, and git.
Provide an LVGL 9.5.0 source checkout before running the full suite:

```sh
rustup toolchain install 1.95.0 --profile minimal
mkdir -p "$HOME/.cache/entropylab"
git clone --depth 1 --branch v9.5.0 https://github.com/lvgl/lvgl.git "$HOME/.cache/entropylab/lvgl"
git -C "$HOME/.cache/entropylab/lvgl" checkout 85aa60d18b3d5e5588d7b247abf90198f07c8a63
export LVGL_SOURCE_DIR="$HOME/.cache/entropylab/lvgl"
bash scripts/test-host.sh
```

The GUI test needs LVGL 9.5.0 sources from the target dependency setup or
`LVGL_SOURCE_DIR`. It links the freshly built unified HEX/Coins/Dice/Words runtime, not historical archives or canned result strings.
Host tests cover the separate bounded upstream extraction (`core-spike/native`),
public vectors, malformed input, CLI guards, HEX contract bounds, real C-to-Rust
linkage and the two public 256-bit regression inputs. Host rendering/geometry
tests are not hardware evidence. See [BUILD](docs/BUILD.md) for pinned target
setup, [RECOVERY](docs/RECOVERY.md) before any separately authorized flashing,
and [CONTRIBUTING](CONTRIBUTING.md).

## Upstream and licensing

Selective reuse, not a full fork: [OogaBoogaX/entropylab](https://github.com/OogaBoogaX/entropylab)
commit `6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51`. The host extraction retains
its exact custom Ooga Booga license; original native wrappers carry MIT notices,
not a blanket license for copied cores, dictionary, fonts or artwork. Credit
**EntropyLab — Team Ooga Booga** and Mr.Hodl’s publicly described
[origin](https://x.com/mrHodl/status/2099170677245014304) and
[calculator scope](https://x.com/mrHodl/status/2099506569931010421), without asserting
sole authorship. The target HEX core is
a direct library adapter, **not** the 15-function host extraction ported to no_std.
The historical `fixture-firmware` directory name is retained for portable build
compatibility. See [provenance](docs/PROVENANCE.md), LICENSE and
THIRD_PARTY_NOTICES.md. Fonts retain their complete OFL notices and provenance;
public vector source licenses are retained in full. No upstream endorsement or
contributor affiliation is implied. Independent source/security and publication
content/privacy/license reviews passed for this bounded source milestone; this
is not cryptographic certification or approval for real-secret use.
The authentic logo is retained under the documented repository-wide coverage
interpretation, not an explicit separate trademark grant or legal guarantee.
The full host suite and published target build script passed. See the
[Words milestone](docs/WORDS-MILESTONE.md) for scoped results and remaining physical
checks. No hardware acceptance or reproducible-build certification is implied.
