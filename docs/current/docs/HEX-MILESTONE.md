> **Historical source-bound record, not current release/feature status.** Evidence and old “current” labels below apply only to the named predecessor identity. For published retention-preview assets, accepted tuple, implemented features and remaining holds see [current status](../../CURRENT-STATUS.md) and [first-install guide](../../PUBLIC-FIRST-INSTALL.md). Current host entrypoint: `python3 scripts/retention-host.py`; predecessor runners intentionally retain their original pins.

# Public-test HEX milestone: evidence and boundaries

## Source identity

Imported onto base `216df64d04f9d84c5510fa1d87903ba63375a03c`.
The reviewed source application SHA-256 is
`2143672c949a6d0380d0e41a9cdadcebce97dae1cf8c2744f9421e1050c24eca`.
The source freeze manifest and this application hash were verified before import.
`HEX-SOURCE-SHA256.txt` records the byte-identical imported application/core/test
files with repository-relative paths. Application GUI, main, headers, Rust source,
Cargo manifest/lock, contract tests, GUI tests and font bytes were preserved.
Only portable build/link integration and documentation change around that source:
archive name `libentropylab_hex_core.a`, `el_hex_run` symbol verification, real
host HEX input, and real-core linkage in the portable GUI CMake target.
No source build scripts, private paths, device identifiers, dumps or logs were
imported. Historical directory/project names are retained, not new features.

## Manual source-hardware observations (user-reported)

The user confirmed these touchscreen observations on the reviewed source build:

1. `Load public zero` loaded 64 zero hex characters (256 bits). Calculation
   displayed 24 words ending in `art`, fingerprint `5436d724`.
2. Replacing the last `0` with `F` (63 zeros followed by `F`) displayed a mnemonic
   ending in `ability trash`, fingerprint `53f6b5aa`.
3. `Clear` removed the input and prior results.

These are user screen observations, not a claim that every displayed word or
address was independently transcribed and checked. The existing label is left
unchanged to preserve tested source identity; it means 64 zeros, not 32.

## Host comparison (separate evidence)

The parent host comparison matched those public-input mnemonic endings and
fingerprints. The portable C harness regression repeats those exact two inputs,
checks 24 words, endings and fingerprints, and checks the mainnet address prefix.
It does not constitute a formally independent cryptographic oracle; shared
libraries and published vectors have different assurance boundaries. The Rust
contract suite separately covers all 24 bundled published BIP39 vectors, strict
input validation, output capacities/sentinels and the published BIP84 fixture.
Output capacities are 216/9/43 bytes including terminators.

The new wrapper regression was first run against the old fixture harness and
failed as expected (`1 != 3`: old output was not the HEX tab-separated result).
The imported code is pre-reviewed source, not a newly implemented crypto feature.

## Local host verification

- `scripts/test-host.sh`: PASS. Existing extraction tests passed in debug and
  release (5 known-answer, 2 CLI, 5 vector tests per profile); HEX contract tests
  passed (4 per profile); C static linkage and the new two-input regression passed.
- Drift suite: all 6 passed when the optional pinned upstream checkout was
  supplied via `ENTROPYLAB_UPSTREAM`. The default run skipped that optional test.
- `scripts/test-gui-host.sh`: PASS with the real HEX core, including keys 0–F,
  lengths 1..65, owned input copy, busy-state protection, invalidation/navigation,
  failure/retry, 12/15/18/21/24 real words, sentinels and **1514 geometry checks**.
- Source-identity manifest, changed-file private-path/device/key-marker scan and
  `git diff --check`: PASS. This is not a substitute for independent review.

## Local target build

The portable `scripts/build-firmware.sh` completed successfully from this
repository with incremental IDF output and external toolchain/dependency caches.
The Rust HEX archive was built from this checkout, and the application was linked
from this checkout; no external prelinked application/core binary was imported.
This is not a fresh-dependency download or clean-room reproducibility claim.

- New application SHA-256: `ebc84180054636911ab4a0b07a88c1d58ee0f55c61c6c2826be48e3d804f05d6`
- New ELF SHA-256: `2d9259c1d864a25fec2704db2a1f432ff5e50862a4a545d5a579e0295d0354b2`
- Verifier PASS: application/bootloader revisions 100..199, configured 200 MHz
  PSRAM, 83 Rust archive members with single-float ABI, linked HEX/allocator
  symbols, forbidden application-path checks and unchanged component lock.
- New binary has **not** been flashed or runtime-tested.

Clear and best-effort wiping do not establish secure erasure. This public-test
calculator is not approved for secrets, signing, persistent storage, RNG or
networking application paths. Independent repository/privacy/license review is
still required before commit/publication; no push, merge or flash is part of this
import. The prior fixture milestone's review does not cover this delta.

## Licensing

The exact upstream custom license, root vector MIT/BSD notices, source provenance,
and complete font OFL notices are retained. Duplicated HEX vectors are
byte-identical to their existing public sources; their licenses apply equally.
Supplemental dependency license texts accompany the import, but do not replace
review of all dependencies before binary distribution.
