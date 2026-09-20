# 460800 baud/progress candidate — software closeout

Subsequent native-platform, packaging, combined-browser and accessibility results, plus the physical port-picker blocker, are recorded in [GATES-REPORT.md](GATES-REPORT.md). The evidence below describes the original software-closeout snapshot.

Base: `124b746ff4cfd5cd878baaaade8e12170ab611ef`. Builds on existing speed commits `9999741` and `0ce4461`; this is not a new implementation. Candidate source only: not published, deployed, flashed, or physically speed-qualified. Accepted v0.1.1 firmware, archive, profile pins, vendor bundle, and held updater remain unchanged.

## Implemented and reviewed behavior

- ROM connection, security checks, and official RAM-stub startup remain at 115200. The actual pinned vendor sends the little-endian baud payload `[460800,115200]`, receives its ACK, and performs the host disconnect/reopen/readLoop sequence before JEDEC and bulk IO.
- The adapter now validates the entire raw ACK before the vendor slices its header: response direction/opcode, actual frame length 10, declared payload length 2, and two zero status bytes. Unsupported switching, malformed/refused/missing ACKs and failed reopen stop before firmware writes. No automatic baud fallback or firmware-block retry was added; existing ROM synchronization/read-only security fallback behavior remains.
- Revoked transports cannot close a successor or acquire its streams. Cleanup waits for an already-started physical close before releasing ownership. Cancellation and deadlines are exercised at all three vendor transition delays.
- All three image and all three erased-tail SHA-256 readbacks remain required, along with each mandatory 16-byte transaction trailer. Success still requires verification and cleanup; no automatic success reset.
- Progress distinguishes compressed bytes acknowledged, bytes read but not yet hashed, and complete SHA-256-verified image/tail bytes. Monotonic elapsed time, per-image denominators, finite phase labels; no invented overall percentage. The 250 ms display throttle is measured from the last displayed event, avoiding starvation under frequent callbacks. This is not a claim of screen-reader usability qualification.
- Terminal/cancelled UI cannot be overwritten by late download, selection, write or diagnostic resolution/rejection, progress, state events, or another Cancel click. Cleanup failure remains actionable. Diagnostics use finite categories and textContent, not raw device/error strings or HTML.
- Minimal CSS wrapping fixes horizontal overflow; no visual redesign.

## Executed evidence

| Gate | Result | Boundary |
|---|---|---|
| `node --test flash/first-install/*.test.mjs` | 62 passed; zero failures/cancellations/skips | Actual handler tests, safety guards, pinned vendor wire paths, 20 raw-ACK/lifecycle tests |
| Chromium transport runner | 16 scenarios passed on Chromium 151.0.7922.34 | Actual vendor/SLIP/native Web Streams, synthetic discovery/device storage; not physical UART |
| Chromium full successful transaction | 3 writes, 52 compressed blocks, 6 SHA-256 readbacks; opens 115200 then 460800; zero resets | Browser protocol evidence, not measured hardware throughput |
| Chromium fault matrix | Refused/short/malformed-header ACK; unsupported switch; reopen failure; post-ACK cancel with successor reuse; stall/cancel/disconnect at baud, write and read phases | Tests assert the fault reached its intended operation; no verified completion on failures |
| Actual UI Chromium runner | 8 cases passed | 320px, 390px, 1280px, CSS 200% layout zoom; success/failure/diagnostic/cancel/incomplete cleanup, safe text and late-event checks |
| Existing onboarding Chromium regressions | Success/failure/diagnostic passed | Public allowlist server; mocked session boundary |
| Public-server regression | 8 allow/deny cases plus installer-asset byte equality passed | Local preview only |
| Current documentation links | 92 links/anchors across 24 documents passed | Local first-party links only |
| Existing `scripts/retention-host.py` full route | Exit 0; 136 Rust test executions passed; native GUI/combined/LifeHash/focused retention completed | Same protected native source; run completed before final browser-only integration, source equality rechecked |
| Direct legacy `scripts/test-host.sh` | Exit 1 at historical GUI identity assertion, also reproduced on base | Not relabeled PASS, repinned, or bypassed by this candidate |

Primary records: `evidence/node-tests.txt`, `evidence/adapter-tests.txt`, `evidence/chromium.json`, `evidence/speed-ui.json`, `evidence/closeout-regressions.json`, and `evidence/host-regression.json`. Independent adapter and integration reviews both passed within their stated scope; their verdicts are recorded in `evidence/review-closeout.md`. Screenshots and verbose native build logs are local review artifacts, not included in the candidate.

## RED → GREEN and ownership resolution

The earlier review caught two defects despite its original passing suite: malformed declared ACK length was accepted, and delayed vendor cleanup could close a successor. Both were reproduced independently before fixes. The added lifecycle tests also caught an already-pending-close ownership race. All 20 focused cases now pass. UI RED → GREEN cycles covered progress starvation, narrow-screen overflow, finite diagnostics, cancelled pending operations and failed cleanup.

Live process inspection found no remaining owner from the historical concurrent-edit warning. Implementation was serialized by file scope; timed-out workers were inspected before takeover. No competing writer is required to finish this candidate.

## Host identity failure explained, not hidden

The legacy verifier expects historical GUI SHA-256 `98db499ddfd502f402016de46968857edf8e388f266ed821e647bbf35af8dd61`. The accepted retention source and archive contain `572bba9eeca5fb1de691acaa494b57207690f75f4cec55dfb38c87b033035ddb`. The relevant later navigation addition is `if(results&&pi_enabled()){pi_enter();return;}`. This same mismatch exists on base `124b746`.

The repository already provides `scripts/retention-host.py` to check accepted source and exact historical inputs separately. That unmodified route completed offline using installed Rust 1.95.0 and locally cached dependencies; final marker: `PASS complete original host chain + focused retention; source745/identity437 unchanged; capture persistence NOT TESTED; firmware NOT BUILT`. Its existing drift-suite skip remains. Historical manifests, firmware source and pins were not changed to force a pass.

## Reproduction

```sh
node --test flash/first-install/*.test.mjs
python3 scripts/build-speed-browser.py
# Use an installed Playwright Chromium; CHROMIUM_PATH optionally selects its executable.
uv run --with playwright python scripts/test-speed-browser.py
uv run --with playwright python scripts/test-speed-ui.py
python3 scripts/test-serve-public.py
python3 scripts/test-current-doc-links.py
python3 scripts/retention-host.py --check
# Full native gate: see docs/retention-host/README.md for compiler/cache/LVGL setup.
python3 scripts/retention-host.py
```

`SPEED_UI_SCREENSHOTS` optionally selects an external screenshot directory. The browser harness generator reproduces the checked-in module byte-for-byte. Browser fixtures use public shipped firmware bytes; readback trailer fixtures establish framing only, while actual pinned SHA-256 checks execute.

## Remaining qualification gates — not software integration blockers

1. Physical public-policy installation and 460800 qualification on the exact board/port: successful ACK/reopen, actual timings, all six SHA-256 readbacks, bounded cancel/unplug/stall handling, then manual RESET and boot/display/touch/public D6/Back retention acceptance. Existing 115200 acceptance is not speed-path acceptance.
2. Native Windows/macOS and broader browser/OS testing; physical accessibility/screen-reader testing and browser-toolbar zoom are not established by CSS zoom/DOM checks. Browser UI and actual-vendor transport are separate harnesses, not one combined app-to-synthetic-device test. Nonterminal state callbacks rely on the current adapter's synchronous phase order; arbitrary reordered callbacks from a future producer are not qualified.
3. Full outside-window flash comparison, recovery/restoration and broader on-device feature qualification remain separate. No new firmware build or reproducible-binary claim; capture persistence was excluded by the existing native gate.
4. Distribution packaging/publication remains separate. `release/public-release.zip` is absent in this source checkout, so the ZIP test cannot run here. No archive was regenerated or published; immutable published assets remain untouched.
5. The separate retention updater and other existing holds remain unchanged.
