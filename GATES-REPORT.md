# Subsequent qualification gates

Runtime under test: `dc5c65420aa96013ae3847ab4771a7f520f1c3a8`. This follow-up adds tests and evidence only; no production app, adapter, vendor, firmware, archive or pin changes. It supersedes the platform/packaging evidence gaps in SPEED-CANDIDATE-REPORT.md, not its remaining hardware/recovery holds.

## Closed automated gates

- **Native macOS:** macOS 26.6.2 arm64, Node 26.6.0, Chromium 151.0.7922.34. All 62 Node tests, 16 actual-vendor browser transport scenarios and 8 application UI cases passed.
- **Native Windows:** Windows 11 10.0.26200 AMD64, Node 24.11.1, Chromium 148.0.7778.96. The same 62/16/8 suites passed. These were native OS executions, not user-agent emulation. Tests still use synthetic serial peers/session boundaries, not physical OS serial drivers.
- Both remote platforms verified all 842 archive files before execution. No runtime file changed after tests; Windows' generated UI report changed its browser metadata, as expected. Controller independently read back native qualification receipts.
- Keyboard Tab/Space/Enter navigation and native Chromium accessibility-tree status updates passed on both platforms. Status is exposed as polite/atomic, with phase and completion text. This is not spoken screen-reader usability acceptance.
- Genuine browser page zoom at 200% passed on both through `chrome.tabs.setZoom`, with getZoom=2, unchanged CSS zoom=1, halved inner width and no overflow. Literal toolbar/accelerator use was not qualified; the attempted shortcuts did not change zoom. The zoom test covers initial layout, not every write/failure scenario.
- **Combined browser integration:** new `scripts/test-speed-app-wire.py` exercises actual app handlers, profiles, GuardedSession and pinned vendor mechanics against the existing synthetic-device subclasses. Only the app's vendor dependency and fixture exports are redirected in memory. No session/profile/adapter methods are mocked. Success, malformed ACK header, refused reopen and UI cancellation during baud all pass. Success performs three image writes and six readbacks. Negative cases assert reached fault/baud history, zero flash begins/data blocks/readbacks, closed streams, no reset or false success. This is not hardware or entirely unmodified dependency injection.
- **Keyboard/cancellation:** new `scripts/test-speed-accessibility.py` passes at 320px and 1280px, using actual app/HTML/CSS with a mocked session/picker. It tests keyboard-only setup/run/cancel, named checkbox AX nodes, polite/atomic status, one-shot operation, and terminal freeze.

## Distribution package — verified, unpublished

Official `scripts/package-public.py` produced an isolated candidate ZIP for runtime commit dc5c654:

- `public-release.zip`: 7,588,712 bytes.
- SHA-256: `a77d0182b88444b1f724ae6f038567678d3e68ac8b13978692fbbbea723e7f38`.
- 842 members; exact tracked path set, no duplicates/traversal/private extra files. All 841 non-self inventory entries and every original member matched size/hash. Repeated generation matched in the same environment.
- Notices/source continuity, documentation successor mutation tests, links, extracted allowlist server and extracted 62-test Node suite passed.
- Controller independently rehashed the ZIP and ran both browser suites from a fresh no-Git extraction: 16 transport and 8 UI cases passed. An earlier optional system-Python Playwright import failure was resolved by using the existing isolated cached Playwright environment; it was not counted as a passing run.
- Accepted source archive remains `82d62829596fe080f6bf02cf67c3337d88d4287a98e6b82ad3c70aa56f8e4f19`; firmware and pins unchanged. This is a new candidate distribution, not a replacement for an immutable published release. No target firmware build/reproducibility claim.
- The ZIP deliberately identifies dc5c654 and does not contain these later test/evidence additions. No publication, tag, release or push occurred.

## Physical board — blocked before port open

The controller verified access to the Dell and the expected single serial interface, no competing accessible serial holder, and the preserved read-only 32 MiB factory backup against its sealed digest. An exact archive was staged under the existing project campaign directory; all 842 files and 11 served runtime assets matched. No private backup or physical identifiers are included here.

The actual candidate app completed image verification in native Dell Chromium, but headless CDP DeviceAccess emitted no Web Serial chooser. The bounded automation stopped before device selection, before its explicit execution token, and before opening the port. Final receipt: **zero write requests, no reset, port free**. This is an automation/permission-UI handoff failure, not evidence of a baud/UART failure. No CLI substitute, automatic retry, or firmware change occurred.

The exact candidate was left on a bounded, loopback-only preview server for a normal headed-browser handoff. The controller independently fetched its app module and matched the reviewed SHA-256. This server owns no serial port and does not automatically install. Its lifetime is limited; do not assume a historical URL is still live without checking.

A normal browser port-selection interaction is still required. After that, the actual app must perform fresh same-session silicon/security/revision/capacity checks, one authorized three-image write, and all six physical image/tail SHA-256 verifications. Only verified completion permits the separately authorized reset. Physical display/touch/public D6/Back retention still requires observation. Historical device identity and earlier firmware acceptance do not qualify this speed path.

## Review and remaining boundaries

Independent review of the two new tests initially found insufficient negative-case assertions. Those were strengthened; final independent review passed after a fresh four-case run and assertion sensitivity controls that rejected an early unrelated failure and unexpected data blocks. Existing runtime review verdicts are unchanged.

Committed evidence: `evidence/remaining-gates.json`, `evidence/app-to-wire.json`, `evidence/keyboard-accessibility.json`. Reproduce the new tests with:

```sh
uv run --with playwright python scripts/test-speed-accessibility.py
uv run --with playwright python scripts/test-speed-app-wire.py
```

`CHROMIUM_PATH` may select an installed Chromium; `SPEED_GATE_OUTPUT` selects an external evidence directory. Defaults use temporary directories and do not dirty the checkout.

Still open: physical 460800 installation and boot/touch acceptance; real serial-driver acceptance on other OSes; screen-reader listening/usability; outside-window whole-flash comparison and actual recovery/restoration; capture persistence and any separate firmware reproducibility claim. The unrelated retained updater's historical holds remain. The original direct legacy host-script identity mismatch remains documented, with the existing full retention-aware native route already passed. Publication remains separate from these automated qualification results.
