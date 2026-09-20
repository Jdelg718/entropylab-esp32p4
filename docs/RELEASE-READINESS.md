# Release readiness — scoped unpublished preview

## Decision and evidence hierarchy

The one-board install/boot/result-retention milestone is accepted, not blocked by the earlier automated port chooser attempt. The current candidate is **unpublished**, with unchanged accepted firmware and archive. No extra flash is needed to repeat that accepted milestone. Runtime `dc5c65420aa96013ae3847ab4771a7f520f1c3a8` is the qualified speed/progress implementation; this documentation/tooling successor does not imply other lanes are integrated.

- [Physical acceptance](../evidence/physical-acceptance.md): human-operated EL-002 installation, browser screenshot showing three image and three erased-tail hashes complete, board photographs and user-confirmed 12/24-word results plus Back/reopen retention. Not a raw serial trace, physical speed measurement or independent result recomputation.
- [Qualification report](../GATES-REPORT.md): native macOS/Windows 62 Node, 16 transport and 8 UI cases per platform; synthetic serial peers, not other-OS physical driver acceptance. The exact prior ZIP was verified, not published; it predates later evidence/test additions.
- [Historical receipt](../evidence/remaining-gates.json): `BLOCKED_BEFORE_PORT_OPEN`, zero writes and no reset refer to the earlier automated attempt only. Preserve that receipt unchanged; it is not the current installation decision. Likewise, the original [software closeout](../SPEED-CANDIDATE-REPORT.md) remains a historical snapshot.

<!-- release-readiness:start -->
| Gate | Evidence state |
|---|---|
| one_board_install_retention | accepted_human_observation |
| six_image_tail_hashes | accepted_browser_result |
| native_platform_suites | passed_synthetic_serial |
| historical_candidate_package | verified_unpublished |
| physical_speed | not_measured |
| outside_window | not_pass |
| recovery | not_qualified |
| final_candidate_package_review | required_before_publication |
| publication_authorization | required_before_publication |
<!-- release-readiness:end -->

## Concrete remaining release actions

For a scoped public-practice preview with these limitations disclosed:

1. **Final source and package review.** The release integrator selects the exact final source commit, reviews changes since the qualified runtime (including any separately integrated lanes), runs identity, docs, Node and affected browser/host regressions, and obtains independent content/privacy/license review. In an isolated tracked source tree, run the official `scripts/package-public.py`, verify the ZIP member set and every member against that tree, inventory hashes and safe paths, then test a no-Git extraction with `scripts/test-public-zip.py` and the affected suites. Record commit, ZIP hash, sizes and results. A prior ZIP's PASS does not certify this successor's contents. Keep accepted source archive, firmware, pins, historical mappings and published assets unchanged.
2. **Explicit release authorization.** An authorized owner approves the exact reviewed artifact and its disclosed scope before any tag, upload or publication. Use a separately versioned package rather than replacing immutable assets. A software test PASS is not publication authorization.

Those are actionable release tasks, not permission to operate hardware. The following remain gates **for broader claims**, not newly imposed requirements to repeat the accepted install:

| Claim not yet supported | Required additional evidence |
|---|---|
| Measured physical 460800 throughput/reliability | Separately authorized raw baud/ACK/reopen and phase timing capture, plus bounded physical cancel/unplug/stall qualification |
| Whole-flash preservation and recovery | Completed independent outside-window comparison and authorized restoration/recovery test; current comparison is NOT PASS |
| Broader physical OS support | Actual serial-driver/device acceptance on each claimed OS; native synthetic peers are insufficient |
| Spoken screen-reader usability | Human listening/usability acceptance; keyboard/AX and API-driven page zoom are narrower |
| Reproducible firmware or persisted GUI captures | Separate target reproducibility and capture-persistence checks; no such claim from the existing native route |
| Broad application or real-secret readiness | Explicit threat-model/readiness decision and feature-specific physical qualification; public practice only today |

The unrelated retention updater remains HOLD. Optional new features, education polish and hardware expansion are not release prerequisites.

## Deterministic offline coherence check

```sh
python3 scripts/check-release-readiness.py
python3 scripts/test-release-readiness.py
python3 scripts/test-current-doc-links.py
python3 scripts/test-documentation-successors.py
python3 scripts/retention-host.py --check
node --test flash/first-install/*.test.mjs
```

The [ledger](../evidence/release-readiness.json) pins the three evidence records and one runtime, preserves the earlier zero-write receipt, and projects an identical bounded gate table into current status, roadmap and this page. Mutation tests reject missing/changed evidence, wrong runtime, promoted speed claims, removed gates, summary drift/duplication and unknown evidence paths. This is an offline consistency test, not a truth oracle for screenshots or a semantic proof of all prose. New acceptance requires explicit review of the contract, ledger and current guidance together, not re-labeling predecessor evidence.

Full native regression remains `python3 scripts/retention-host.py` with its [prerequisites](retention-host/README.md). The mandated legacy `scripts/test-host.sh` rejects historical GUI identity on this successor; preserve and report that failure, never repin it to claim PASS. Historical evidence and current execution results must be reported separately.
