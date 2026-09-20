# Speed candidate handoff

Base fetched from live GitHub main: 124b746ff4cfd5cd878baaaade8e12170ab611ef.
Scope: existing public first-install adapter, not a greenfield flasher. No firmware build, device access, SSH, flash, deploy, or publication performed.

## Implementation and TDD
- ROM constructor and connection remain 115200. After security and official stub checks, bounded changing-baud phase calls the actual pinned vendor changeBaud with selected baudrate 460800.
- Actual vendor sends ESP_CHANGE_BAUDRATE with little-endian [460800,115200], waits for response, then delays/transport.disconnect/connect(460800)/readLoop. Vendor itself only uses command, not checkCommand: adapter validates exact two-byte successful stub status before permitting reopen. Failed/unsupported switch aborts, no automatic fallback or firmware retry.
- Existing absolute operation/phase checks and transport guards remain. Existing three image plus three FF-tail SHA-256 checks and mandatory 16-byte transaction trailers remain. Vendor bundle, firmware assets, archive, and pins unchanged by this work.
- Progress callback distinguishes compressed-write acknowledgements, unverified image-read bytes, and SHA-256 image-verified bytes. Monotonic elapsed values, no overall percent. PROGRESS-CONTRACT.md saved early for separate UI worker.

Observed RED→GREEN cycles (executed, not inferred):
1. Wire transition test initially failed undefined !== 1 (no opcode), then passed with actual vendor switch.
2. Refused ACK test initially failed Missing expected rejection; after status validation, passed with no reopen/write.
3. Progress test initially failed 0 !== 6; after adapter events, passed with six verified events and compressed byte callbacks.

## Reproduction
`node --test flash/first-install/wire.test.mjs flash/first-install/guards.test.mjs flash/first-install/public.test.mjs`

`uv run --with playwright python scripts/test-speed-browser.py`

Browser runner accepts CHROMIUM_PATH for an already installed executable. Chromium 151.0.7922.34 executed actual bundled Transport.readLoop/read/write framing with native ReadableStream/WritableStream, real vendor stub upload/write compression/readback code; simulated discovery and public-memory chip endpoint. Full success: three writes, six SHA-256 readbacks, 52 compressed blocks, one baud command, opens at 115200 then 460800, zero resets. Fault paths in evidence/chromium.json. This is browser protocol evidence, not physical speed or UART qualification. Trailer fixture is framing-only; SHA-256 remains authoritative.

`scripts/test-host.sh` attempted and blocked at pre-existing source identity assertion: review-repair current mismatch: fixture-firmware/app/main/gui.c. This work does not modify firmware source. No repository-wide PASS claimed.

## Integration caveat — historical, resolved
The earlier concurrent-edit warning applied to the intermediate handoff, not an active owner. Live ownership was checked before serialized completion. The integrated adapter/UI now passes 62 Node tests, 16 Chromium transport scenarios and 8 Chromium UI cases, with separate independent adapter and integration reviews. Raw ACK framing and late cancellation lifecycle issues found after the initial handoff are fixed. See [current closeout report](SPEED-CANDIDATE-REPORT.md) for final evidence, the completed retention-aware native host gate and remaining physical/distribution qualifications. Held profiles remain unchanged.

## Hardware qualification checklist — not executed
- Preserve physically accepted v0.1.1 / 115200 baseline and unchanged archive/pin hashes.
- Explicit board/port/destructive consent; confirm ROM security, chip/revision/JEDEC gates.
- Confirm ROM and stub at 115200, successful ACK then host switch 460800 before bulk writes/readback.
- Record actual phase durations and bytes; no claim of fourfold physical throughput.
- Confirm three image SHA-256 and three full FF-tail SHA-256 readbacks before completion; write 100% is not verified.
- Qualify rejected baud, unplug, cancellation and stall: bounded cleanup, no automatic retry/reset; recovery requires a new authorized session.
- Manually reset only after completion; verify boot/display/touch, 24 words, expected public fingerprint and Back retention using the accepted public test procedure. Do not inherit EL002 hardware acceptance for this new speed path.
- Keep protocol/private/public holds and publication decisions unchanged until independent review and hardware acceptance.
