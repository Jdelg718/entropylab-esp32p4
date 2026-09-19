> **Current guidance:** [PUBLICATION-STATUS — current retention preview](current/docs/PUBLICATION-STATUS.md).
> The text below is the historical source checkpoint, not current release status.
> v0.1.0-retention-preview is published; v0.1.1 is a local documentation/onboarding patch pending publication.
> Firmware is unchanged; public-policy hardware qualification and the separate retention updater remain held.

# Source preparation complete — publication HOLD

Proposed branch: `release/d6-review-edit-20260917`. This preparation did not create a branch, commit, push, merge, tag, release or Pages deployment.

The complete `bash scripts/test-host.sh` gate passed (inner exit 0) on a fresh source-only copy of combined manifest `0c11e6fd9fcccc260f78918367c588c5095c755382cad35920a5c88a3ff7de10`. It compiled fresh host artifacts, ran the default GUI and all ten pointer-family slices, and continued through the combined native suite. Rust 1.95.0, GCC 14.2.0 and CMake 3.31.6 were inspected; existing LVGL sources and an isolated copy of the dependency registry were reused. This is clean-source/build-directory host assurance, not a fresh-network dependency test, ESP32-P4 build, binary reproducibility certification or hardware acceptance. Upstream dependency compiler warnings were retained, not represented as warning-free success.

The same combined source received bounded independent source review: 37 focused tests and 74 independent semantic probes (three accepted valid controls, 71 rejected invalid mutations). All original findings are closed for that reviewed code. This package changes documentation/status only after those gates; `HOST-VERIFICATION.json` binds the tested manifest and stream hashes. Production, tests, scripts, lockfiles and historical successor manifests remain byte-identical to the tested package.

Complete target image/ELF/map/resource/layout verification, exact pushed-SHA CI, first-install/update/recovery tests, physical acceptance and release/flasher review remain separate pending gates. No release, download URL, installed identity or publication approval is conveyed.

## Source identity

`MODAL-COMBINED-SOURCE-MAPPING.json` is the current 96-file frozen-source mapping: 91 unchanged, five explicit successors (three production and two host-test files). Historical mappings and successor manifests are retained rather than rebaselined. `PACKAGE-SOURCE-MANIFEST.json` identifies this documentation-only closeout package and excludes itself.

## Historical failures — preserved, not current status

An initial clean-host attempt was blocked by disk exhaustion. After space was restored, the original frozen D6 package compiled and linked but failed the D6 raw pointer-navigation assertion (exit 134). The review-repair predecessor still had that failure. The explicit modal successor repaired visible method reachability, and the combined full host gate above subsequently passed. Original failure receipts remain private evidence; no failure was relabelled as a pass.

## Interaction boundary

Selecting Dice now opens the method chooser instead of immediately resuming the remembered method. There is no Back-to-family action; Close returns to the underlying screen. Host tests cover real pointer navigation and existing destructive-change confirmations, not exhaustive all-owner state combinations or hardware touch acceptance.
