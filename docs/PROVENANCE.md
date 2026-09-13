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
  secp256k1 0.29.1, bip39 2.2.2. Both Cargo.lock files retain exact checksums.
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

The target fixture adapter and C UI were copied from the hardware-tested narrow
fixture. They do not depend on the host extraction. Checked display/backlight/queue/
task/lock startup is retained in main.c. Source build caches, giant upstream trees,
board benchmark apps and unfinished GUI prototype are intentionally excluded.
