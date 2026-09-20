# Independent software closeout reviews

Controller verified reviewed file fingerprints against final source; see source-integrity.json. Reviewer scope limits are retained verbatim below.

# Final independent adapter review — PASS (bounded host-only)

Reviewed the working candidate at `candidate checkout` against `124b746`, including the untracked `flash/first-install/baud-lifecycle.test.mjs`, the earlier HOLD report, and the actual imported pinned vendor implementations. **No blocking findings remain in the reviewed adapter scope.** This is not a UI, browser, hardware, target-build, or publication approval.

## Executed evidence

Command, run from the candidate checkout:

```sh
node --test flash/first-install/baud-lifecycle.test.mjs flash/first-install/wire.test.mjs flash/first-install/guards.test.mjs flash/first-install/public.test.mjs
```

Exit **0**; **45 tests passed, 0 failed, 0 cancelled, 0 skipped**; duration approximately 9.38 seconds. This includes all 20 lifecycle tests. Tests were run independently for this review, not inferred from the fix worker's report.

## Earlier blockers and lifecycle races

- **Malformed raw ACK: fixed.** `flash/first-install/adapter/adapter.mjs:235–252` validates the complete SLIP-decoded response before vendor `readPacket` discards the header: actual length 10, response direction 1, matching change-baud opcode, declared payload length 2, and two zero status bytes. The command-boundary decoded-status check remains. Twelve malformed-frame cases traverse actual vendor SLIP/readPacket and fail before baud reopen, firmware write, or reset (`baud-lifecycle.test.mjs:58–80`). The exact successful raw ACK passes the actual transition (`:81–87`).
- **Cancelled old vendor close disrupting a successor: fixed.** Guarded `close` checks revocation before touching the physical port (`adapter.mjs:160–164`). Cancellation/deadline tests pause each of the actual vendor's three transition delays, start a successor after cleanup, and then release the old continuation. All six cases preserve successor streams and open/close counts with no writes, resets, or captured unhandled rejection (`baud-lifecycle.test.mjs:100–137`).
- **Already-started close: accounted for.** An authorized close promise is recorded (`adapter.mjs:162–163`); cleanup waits for it before the final close and removal from `activePorts` (`:114–126`). The deferred physical-close test proves cleanup does not resolve ahead of that pending operation (`baud-lifecycle.test.mjs:89–98`). Static review also confirms the cleanup timeout returns failure without itself deleting port ownership; deletion remains after physical cleanup. The timeout variant of this deferred-close test was not separately executed.
- **Late unawaited readLoop: fixed.** Revoked proxies return null for readable/writable streams (`adapter.mjs:155–159`). The actual vendor `changeBaud` launches `readLoop()` without awaiting it after its third delay; its loop condition is `this.device.readable`. Thus the stale loop exits without touching successor streams. Both cancellation and deadline cases at delay 3 exercise this with native Node Web Streams.

The pinned vendor methods were inspected directly via imported method source: ACK → delay → disconnect → delay → connect → delay → unawaited readLoop. Vendor `readPacket` still ignores declared payload length; the adapter now closes that gap specifically for the baud ACK.

## Preserved trust boundary

- Exact `FACTORY_PROFILE` declaration matches baseline `124b746` byte-for-byte; vendor and firmware directory diffs against that baseline are empty.
- Independently recomputed SHA-256 for all three shipped images and all three expected erased-FF tails; all six match their pins:
  - boot: `530330004af627bf2de7ac9b1d05ab34f8a655829a74005b96fc15dd7083f678`
  - table: `d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb`
  - app: `a02a2192b5c302390a916f9ee67961b699e5f2f9038135378eed0b19d792f33b`
  - tail at 29456: `1fa7330b398eac6db72a2e7a012308f62f3e75035fb3164acbea583f011a26e8`
  - tail at 35840: `5f4ecdb7b71c3e403983fe405cddcdc2f2576b655fdb3e80d94a6f7c32e58bc2`
  - tail at 1605488: `d169f6754229c200ba4838a38e4894c03c47ce8939db4b7a11c224921b982521`
- Board confirmation/destructive consent and immutable input-copy/hash gates remain before port opening (`adapter.mjs:196–209`). Security frame validation and chip/security/revision checks are unchanged; security and official stub precede baud switching (`:214–230`). Guard tests pass.
- Required 16-byte raw readback trailer remains consumed and checked before returning readback bytes (`:25–30`); progress forwarding does not replace it or SHA-256 verification. Each of three images plus three tails remains hashed against the trusted pins before success (`:260–279`). Actual vendor wire tests verify three writes and six readback transactions, including fail-closed tail corruption.
- Firmware block attempts remain one; eraseAll remains false; no new network, secret, consent bypass, pin change, or raw dependency-error disclosure in the adapter diff. Profiles wrapper only forwards the new progress callback.

## Reviewed snapshot and scope

SHA-256 fingerprints of reviewed files:

- `flash/first-install/adapter/adapter.mjs`: `783533cc8b2a13b9315d1743a705c807be4e2d9ff432356517ba4fc885704b11`
- `flash/first-install/baud-lifecycle.test.mjs`: `a728e892641ecb98cfa04062c31c8da1c9e727d6cede17cbc939961de2ae3587`

No repository files were modified. Only this external verdict was created. Active UI/app/completion work was excluded. No firmware build, SSH, attached hardware access, target build, publication, or full-repository validation was performed. The passing lifecycle tests use a synthetic physical port and preflight with the real pinned vendor transition/protocol/stream machinery; they are not hardware evidence.


---

# Final independent integration/spec/privacy review — PASS (bounded)

Reviewed the final working candidate against `124b746`. **No blocking findings in the current shipped integration.** This is host/browser synthetic evidence only, not hardware, boot, recovery, target-build, publication, or full-repository approval. The separate final adapter PASS was read; its full transport/security research was not repeated.

## Independently executed

- `node --test flash/first-install/*.test.mjs`: exit 0, **62 passed, 0 failed/cancelled/skipped**. Includes completion cancellation matrix, progress, public policy, guards, wire and lifecycle tests.
- `CHROMIUM_PATH=… uv run --offline --with playwright python scripts/test-speed-ui.py`: exit 0, **8 cases passed** in **Chromium 151.0.7922.34**. Actual app/HTML/CSS with simulated session boundary and real local image verification. Widths 320, 390, 1280, plus 1280 at CSS zoom 2: scroll width equals client width; status role present. Four success/layout cases and failure, diagnostic, cancellation, cleanup-incomplete cases passed.
- Same offline Chromium invocation with `scripts/test-speed-browser.py`: exit 0, **16 scenarios passed their assertions**, using imported pinned vendor loader/transport and native browser streams around a synthetic device. Success: three writes, six readbacks/verified spans, rates [115200, 460800], no resets. Fifteen negative scenarios correctly did not succeed; partial-write/read and late-ACK successor checks passed.
- Executed builder transformation in memory with its final write replaced by an equality assertion: generated `speed-browser.mjs` matches `scripts/build-speed-browser.py` byte-for-byte. No generated source was rewritten.

## Findings and requirement checks

- **Cancellation freeze PASS:** `app.mjs:42–44` checks `ended` after awaited preparation, selection, run and diagnostic, including catches; completion-finally cannot append late text after cancellation. `:47` sets cancellation text before awaited cleanup, makes cancellation one-shot and only refines that text when cleanup fails. `completion.test.mjs:6–13` independently passed all four pending stages × resolution/rejection. Cleanup false/rejection tests also passed. No raw rejection messages surface.
- **Terminal protection PASS:** `app.mjs:9–11,24–25` blocks subsequent state/progress once terminal or ended; `:44–47` preserves completed/failed/diagnostic terminal outcomes from later cancel clicks. Browser terminal tests inject late progress/state and confirm frozen text. Success-only manual RESET instructions remain separate from failure/diagnostic/cancel wording; no automatic reset added.
- **Progress semantics PASS:** `app.mjs:24–38` validates kind/index/bytes/time/phase, separates compressed acknowledgements, received-but-SHA-pending bytes and SHA-verified spans, omits unknown totals and invents no aggregate percent. Ordered progress rejects earlier assets/kinds, decreasing bytes within a span and elapsed-time regression; display throttling is measured from the last display rather than every received event. Full spans bypass throttling. Actual adapter calls retain serial image-then-tail order and only report verified after SHA match.
- **Finite diagnostics/privacy PASS:** `app.mjs:6,13–20` renders textContent and allowlisted diagnostic values, bounded ECO integers, finite error/reason labels, no raw message/frame/error detail. Hostile markup and private-marker rejection tests pass. Reviewed changed/untracked text artifacts contain no discovered absolute private filesystem paths, bearer credentials, private-key material or added telemetry/storage hooks. Only loopback URLs appeared in the scan. Public fixture builder reads the public Node fixture, not backups or private predecessor material.
- **Policy preservation PASS:** `adapter/profiles.mjs:2–5` only forwards onProgress and continues refusing every mode other than publicfirstinstall. Board/consent button gates and explicit consent argument remain in `app.mjs:40,44`; existing guard/public tests pass. The baseline diff is empty for index.html, assets.mjs, firmware and vendor directories. Independent adapter report confirms FACTORY_PROFILE byte-for-byte preservation.
- **Browser evidence honestly scoped:** `scripts/test-speed-ui.py:15–21,65` explicitly labels **CSS 200% layout zoom, not browser toolbar zoom**. `speed-browser.mjs:16–59` uses a synthetic port/ROM device, while actual vendor baud/write/read/SLIP stream mechanics execute; reset/sync, JEDEC and selected device responses are simulated. `:68–95` enumerates and asserts all 16 scenarios. Neither test supports hardware or actual baud-throughput claims.

## Nonblocking gaps / precise scope limits

1. `app.mjs:9–22` has no independent monotonic rank for arbitrary nonterminal **onState** injection, nor a shared phase cursor with onProgress. Current adapter emits states synchronously and in order, and progress events themselves have monotonic ordering. Thus no reachable regression was found in this integration. If the requirement is expanded to tolerate arbitrary reordered state callbacks from an untrusted/future session producer, this needs an additional guard/test; do not claim that stronger property. The UI phase-label test (`scripts/test-speed-ui.py:27–28`) deliberately injects readback state then synthetic write progress, so it proves labels, not general cross-channel state ordering.
2. UI Chromium tests mock the session and wire Chromium tests bypass the UI. Together with wrapper inspection and Node tests this covers the present boundary, but it is not a single end-to-end app-to-synthetic-device browser test. Toolbar zoom, screen-reader announcement quality, and visual screenshot inspection were not tested; DOM layout/status-role assertions were.
3. Generated harness retains unused fixture scaffolding (`speed-browser.mjs:9,14,63`, including a dormant clock reference for an unused option). The 16 enumerated scenarios do not activate it; this is maintenance debt, not a production or tested-path blocker.
4. This bounded review did not run scripts/test-host.sh or firmware/target builds, and does not constitute a comprehensive secret-history audit. No implementation changes were made, so no broader rebuild was attempted.

## Snapshot fingerprints

- app.mjs: `05c6a4dc2275683058c4519c1488eac4f2e54ae8ccdfb07f5842bcdd495098af`
- adapter/profiles.mjs: `5977fa70eb794a4c9849323e4337fba76c71296792e6e832ae21bf60accade4e`
- style.css: `c86bf73500582be162d7015704bac15b1ab22f623d4caa8c2ad0a5f0fe402662`
- speed-browser.mjs: `a624e7d596fc2806103d6f3c285e0ea405d6df16c767ed10ab425fb648ed2023`

Only this external review was created; the explicitly authorized UI replay rewrote evidence/speed-ui.json. No implementation edits, commits, firmware changes, SSH, attached hardware access, external network requests or publication occurred. Browser test servers were loopback-only.
