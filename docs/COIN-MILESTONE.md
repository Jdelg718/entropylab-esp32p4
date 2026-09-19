> **Current guidance:** [COIN-MILESTONE — current retention preview](current/docs/COIN-MILESTONE.md).
> The text below is the historical source checkpoint, not current release status.
> v0.1.0-retention-preview is published; v0.1.1 is a local documentation/onboarding patch pending publication.
> Firmware is unchanged; public-policy hardware qualification and the separate retention updater remain held.

# COIN source import — local review candidate

Base: `a1f71c0fe73ad3eebc92d13c579bb10f881ea64f` (clean main).
Reviewed source application image SHA-256:
`74061341e2ea3e680b228d0f647c32680973f35722375eab47e9decfa3965a6b`.
The source import manifest records relative source identities and exact original
and imported hashes; these are provenance, not proof of independent review.

## Hardware observations (source image only)

User confirmed **Coins / 12 / Load public zero / 128 bits**, fingerprint
`73c5da0a`, and **Coins / 24 / Load public zero / 256 bits**, fingerprint
`5436d724`, both good on board. No manual-flip, Undo, or mode-isolation hardware
PASS is claimed. Those are host test properties only. The new repository-built
binary has NOT been flashed or exercised on hardware. No funds or real secrets.

## Integration changes

Retain the existing HEX Rust source, dependency lock, fonts and licenses.
Import the reviewed COIN GUI, worker computation header, adapter and contract tests.
The `lvgl.h` include moves from the request/result header into the GUI consumers:
non-GUI worker tests compile without LVGL; request/result layout and logic unchanged.
The worker fixture path becomes a command-line argument, removing checkout-layout
coupling. Adapter test paths now reference the existing `../rust` core. CMake and
portable scripts build both archives from source, never copy prelinked libraries.
Host Rust is pinned to 1.95.0; target Rust to nightly-2026-04-15; existing IDF pin
and component lock remain authoritative. External toolchain/cache reuse does not
import vendor sources or archives into tracked content.

Target adapter panic localization retains the original archive, checks exactly
one global removal and no additions, local panic presence, unchanged allocated
payload, and strict relocatable composition. HEX remains the global panic owner;
final ELF verification checks panic abort and allocator/entrypoint symbols.
No duplicate-symbol relaxation is permitted. Assertions stay enabled in host GUI
and worker tests. The fresh target image need not equal the source image: source
paths, linkage and build identity differ; do not assert binary equivalence.

## Fixture and license provenance

`fixture-firmware/coin/vectors/raw-binary.json` records upstream commit
`6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51`, app hash and exact source ranges.
The 22 raw-binary fixtures are an exact filtered projection of the retained
`upstream-expanded.json`; `verify-fixtures.py` verifies JSON, TSV and inherited
lock checksums, not fresh upstream execution. They are derived from upstream
OogaBoogaX/entropylab and retain its custom LICENSE-OOGA-BOOGA, not a fabricated
MIT attribution. Adapter LICENSE is copied verbatim. Inherited HEX dependency
licenses and root third-party notices still apply; Liberation-derived fonts
retain OFL and source provenance. No blanket relicensing or complete binary
license-compliance assurance is claimed.

## Verification commands and release gate

Run `bash scripts/test-host.sh` (includes standalone worker without LVGL), then
`LVGL_SOURCE_DIR=<LVGL-9.5.0-source> bash scripts/test-gui-host.sh` and, after
exporting the pinned ESP-IDF toolchain, `bash scripts/build-firmware.sh`.
These execute the real composed core, all 22 upstream coin vectors, all five
word widths, all displayed words, busy guards, mode isolation and invalidation,
plus existing HEX and extraction contracts. Consult actual process outcomes;
commands documented here are not PASS claims.

## Executed verification

Full `test-host.sh` and real-LVGL `test-gui-host.sh` completed exit 0.
The GUI reported 1668 HEX geometry checks plus COIN all-selector, mode-isolation,
busy, retry and actual-core result checks. The standalone C worker compiled
without any LVGL include path and passed the 22-vector composed-core contract.
The target build and post-build verification completed exit 0, including strict
panic localization, allocated member payload comparison and final ELF checks.
New unflashed artifacts (ignored `fixture-firmware/build`):

- Application SHA-256: `80fb5528bdc6fdded9fe5b24889e4dd6a2aec18ce9a3c657c1d83eddae3995a0`
- ELF SHA-256: `b8bba086d46c7fe1bddf32f3a07bc8c41adc1202a211d35b44517063789fae65`

Initial worker compilation exposed an unnecessary LVGL header dependency; fixed
by the include move above. Initial payload verification attempted unsupported
whole-archive binary conversion; replaced with ordered per-member ELF allocated
section comparison (names, type, flags, address, length, bytes), preserving
duplicate member ordering without extracting untrusted names. The focused
`python3 scripts/test-coin-archive.py` regression passed: real GNU ELF32 host
fixtures accept intended panic localization and reject allocated-byte mutation,
unexpected global addition/removal, missing/extra/reordered ELF members, and
malformed archive output (eight subcases). Each rejection requires nonzero exit,
no PASS message, and no traceback; payload/member mutations additionally require
the allocated-payload diagnostic. Tests use temporary fixtures and matching GNU
assembler/binutils from PATH, overridable with `COIN_TEST_AS/AR/NM/OBJCOPY/LD`;
missing tools explicitly skip. No target compiler is required by these tests.
RED exposed uncaught verification errors; GREEN adds a bounded CLI error handler
and refuses optimized Python rather than silently disabling assertions. No
payload or application logic changed. Existing real target archives were checked
again with strict composition, and existing final ELF verification passed; no
target rebuild or flash was performed. The regression is now in `test-host.sh`.
This focused follow-up's full host rerun first encountered non-rustup PATH cargo;
explicit `CARGO="$CARGO_HOME/bin/cargo"` passed debug native/HEX tests but hit the
bounded run timeout during release compilation, so no fresh full-host PASS is
claimed here. The earlier completed full-host result above remains historical.
Manifest verification and `git diff --check` passed. Tracked/candidate sources
contain no local absolute workspace/home paths. Existing HEX Rust, extraction
sources and component lock remain unchanged.

LOCAL ONLY: no flash, commit, push or merge before independent SPEC/security
review. Build products, logs, vendor components and tool caches remain ignored.
