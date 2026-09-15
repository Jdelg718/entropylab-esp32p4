# Provenance and reproducibility

- EntropyLab: https://github.com/OogaBoogaX/entropylab at
  `6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51`.
  `core-spike/native/src/upstream_core.rs` extracts 15 routines from
  `entropylab-wasm/src/lib.rs`; function bodies unchanged, imports subset and
  Rust visibility/ABI adjusted. `core-spike/vectors/extraction.json` records source
  SHA-256 and function names. Exact custom license retained.
- Waveshare baseline reference: https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3
  at `dc1b2085381c9cc3a2badf8d317673cca9079c1f`. No vendor tree bundled.
- ESP-IDF v5.5.5: https://github.com/espressif/esp-idf at
  `b774170ff46c393eeb5e495ea37936038d3f4f4f`.
- Registry BSP `waveshare/esp32_p4_wifi6_touch_lcd_4_3` 1.0.1,
  LVGL 9.5.0, and all resolved component hashes: app/dependencies.lock under fixture-firmware.
- Rust host 1.95.0; embedded nightly-2026-04-15 with rust-src,
  riscv32imafc-esp-espidf, core+alloc, static final C linkage, ilp32f.
- Direct Rust versions: bitcoin 0.32.11, bitcoin_hashes 0.14.101,
  secp256k1 0.29.1, bip39 2.2.2. The retained Cargo locks record exact checksums; the unified runtime lock
  governs new target/GUI builds, not nested historical standalone locks.
- Published vector sources: https://github.com/bitcoin/bips (BIP32/BIP84),
  https://github.com/trezor/python-mnemonic (vectors.json). Preserved exact snapshots
  are hash-pinned in core-spike/vectors/sha256.json; retrieval commits were not
  recorded, so snapshot hashes, not invented source revisions, identify them.

## Reproduce extraction (optional, separate from normal host tests)

Clone EntropyLab into an external cache, checkout the exact commit above, and set
`ENTROPYLAB_UPSTREAM` to its absolute path. Run `python3 core-spike/extract.py` and
`python3 core-spike/test_drift.py`. The extractor validates both revision and source
worktree before writing. Normal host tests need no upstream tree; only the upstream
worktree-drift test skips without that optional checkout. Fixture-hash drift tests
still run. Do not regenerate trusted manifests to make a mismatch disappear.

## Words publication lineage

The candidate follows public Dice base
`8e5e721c31e7d2aa09e093e7b4dd8d5bc39db1de`. The application imports authenticated
installed-source Words prefix selection, taller editor, Orbit, banner cleanup,
practical selected-mode Clear and Safety/About. Four source cores are copied into
`fixture-firmware/runtime-sources`; `fixture-firmware/runtime` compiles their
unified graph afresh. This target implementation does not depend on the separate
15-routine host extraction. No historical prebuilt archive supplies the runtime.

Source authentication is not binary identity: the new layout/build has its own
identity. The earlier installed image's recorded app readback and boot do not
prove this candidate was flashed or physically accepted. Full host tests and the
published target build script passed; see [Words milestone](WORDS-MILESTONE.md).
No reproducible-build or secret-erasure certification is claimed.

Dictionary origin/license is retained in `fixture-firmware/LICENSE-dictionary.txt`;
fonts retain their adjacent OFL notices and provenance. Authentic Orbit artwork
has [source-local provenance](../fixture-firmware/app/main/assets/PROVENANCE.md)
and a relative-path hash manifest. It is retained under a repository-wide coverage
interpretation supported by upstream README lines 570–575 and the integrated
tracked asset, not an explicit separate trademark grant or legal guarantee.

Credit **EntropyLab — Team Ooga Booga**, upstream, and Mr.Hodl's public
[origin account](https://x.com/mrHodl/status/2099170677245014304) and
[calculator scope](https://x.com/mrHodl/status/2099506569931010421); no sole authorship,
affiliation or endorsement is implied. Exact custom and third-party license texts
remain authoritative; see [notices](../THIRD_PARTY_NOTICES.md).

Private evidence, device identifiers, backups, user input, build caches and vendor
trees are excluded. Source, host checks, target build, installed identity and
physical acceptance remain separate evidence layers.
