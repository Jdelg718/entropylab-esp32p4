# Accepted retention: portable host gate

From a clean clone (Python 3.12+, Rust 1.95.0, C/C++, CMake, pinned LVGL 9.5.0):

```sh
export LVGL_SOURCE_DIR=/your/dependencies/lvgl-9.5.0
python3 scripts/retention-host.py --check
python3 scripts/retention-host.py
```

CI uses this same command. Cargo uses locked dependencies; a populated cache may
use `CARGO_NET_OFFLINE=true`. Optional CARGO, CARGO_HOME, RUSTUP_HOME, BUILD_JOBS,
and CARGO_BUILD_JOBS are accepted. Reserve several GiB of temporary disk space.
No target build, hardware access, flashing or publication occurs.

## Why a separate entrypoint?

The inherited `scripts/test-host.sh` and `scripts/d6-release/test-host.py` bind
predecessors. Their gui.c rejection is correct for those historical identities,
not import corruption. No historical manifest, source pin or driver is repinned.
The archive has a wrapper directory and a `source/` subtree, not a flat checkout.
The new runner verifies the pinned complete archive using its original portable
validator, then compiles the accepted `source/` subtree in disposable scratch.
All 745 archived files and all 437 target-identity paths are verified, including
sdkconfig. The checkout must match every accepted file except README.md and the
CI workflow, which are explicitly packaging-only changes. The complete accepted
README and original CI bytes remain in the archive and are used in scratch.
This is source identity to the packaged accepted application, not a reproducible
firmware-build claim or a new attestation of installed hardware state.

## Finite successor and historical proof

The archive's independently pinned validator checks five modifications:
`app/main/gui.c`, `app/main/passphrase_gui.inc`,
`app/main/passphrase_integration.inc`, `tests/gui_host.c`, and
`tests/passphrase_gui_tests.inc`, all under `fixture-firmware/`.
The sole addition is `fixture-firmware/tests/dice_navigation_tests.inc`.
Original predecessor and candidate hashes, rationale/classification, target
identity, source allowlist, original744, recipe, historical records, licenses
and accepted app binding are all preserved inside the archive. App SHA256:
`a02a2192b5c302390a916f9ee67961b699e5f2f9038135378eed0b19d792f33b`.
Target source identity SHA256:
`5d730162bc966e98c45868cdc5acf1414f63a2abecfa20c2390dd6cdcd640778`.
Source allowlist SHA256:
`7a9e89b51c433ccccc5594e5c376e03193f39e5a0bd285c8b6d1ca4e7cfabb10`.
Public inventory anchor:
`3110081933447e5cb0e7e7d2e326a089701b2cba74e4a1341a5152f66765a7e9`.

`historical.json` pins the separate 436-file historical projection. The eight
changed predecessor inputs in `inputs/` reconstruct that projection; the added
retention test is absent there. Six historical identity/adversarial tests run
against that tree. No executable current-source tests run against old code.
Only shell ROOT, six historical Python paths, nested shell paths and direct
GUI capture destinations are adapted in temporary copies. Original debug/release,
C ABI, coin, dice, words, default GUI, all ten modal families, LifeHash including
sanitizers, and combined application stages retain their order. Focused retention
runs additionally after the full original chain. Unknown inherited selectors
are excluded by environment allowlisting. Optimized Python is rejected.

Direct gui_host children redirect only writes to `.ppm` files to `/dev/null`;
rendering and assertions remain active. Capture persistence is **not tested**.
Any nonzero test exit fails the command; timeout or interruption is **not PASS**.
A final source-hash check is required before the success line. This integration
changes host packaging only; release hold and original firmware remain unchanged.
