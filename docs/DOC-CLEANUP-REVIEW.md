# Documentation/onboarding patch — parent review only

Based on public main `423702a7746cc9d301aeb9459172c6988992d007`. No publication, tag/asset replacement, deployment, hardware work, firmware build, speed or security/protocol change.

## Finding disposition

1. Addressed: release-first README; original historical text preserved separately.
2. Addressed via current CONTRIBUTING companion linked from README: main/tag and retention host commands. Pinned original unchanged.
3. Addressed via current BUILD/BUILD-FLASH/KNOWN-LIMITS companions, not rebaselined originals.
4. Addressed via current PUBLICATION/HANDOFF/MODAL companions with historical context.
5. Addressed via current ROADMAP companion: docs/onboarding, paused 460800/progress, education, remaining verification.
6. Addressed: published first-install status and precise remaining retention updater hold.
7. Addressed: historical updater branding/link/hosting explanation; guards unchanged.
8. Addressed via current CHANGELOG companion: actual prerelease date and tuple boundary.
9. Addressed via README/CURRENT-STATUS feature map and current SECURITY/UPSTREAM companions; originals remain evidence.
10. Addressed via contextual provenance/release-hygiene/D6 companions and bounded installer-review context.
11. Addressed: README plus current HARDWARE companion distinguishes PCB from silicon.
12. Addressed: adapter comment only; no baud, timing or behavior changes.
13. Addressed: published checksum layout explained in CURRENT-STATUS; no old assets replaced.
14. Addressed: HTML-adjacent notice context; concatenated notice/license bytes untouched.
15. Addressed: CURRENT-STATUS explains later portability successors outside pinned adaptation source; original untouched.
16. Addressed via current milestone companions; original milestone evidence unchanged.
17. Addressed via current SECURITY companion: no documented private reporting channel; no invented contact.
18. Addressed: successful app run only gets manual RESET/boot/D6 guidance, no reinstall suggestion; diagnostic and failure remain separate. resetOnSuccess remains false.

## Identity constraint and review caveat

The accepted source allowlist pins most historical root/docs files too, not just firmware. Corrected companion files under docs/current preserve that constraint without weakening scripts/retention-host.py. README and CURRENT-STATUS route readers to these companions. GitHub's automatic CONTRIBUTING/SECURITY discovery still opens the pinned originals: parent must decide whether this companion approach is acceptable or authorize a separately reviewed packaging-identity mechanism. Do not describe that residual discoverability issue as solved.

## Verification and remaining gaps

Accepted archive745/identity437/checkout/historical436 verification passed after preserving all pinned inputs. Exact notices282 and accepted fixture source330 equality passed. The original 22 Node adapter/wire tests and public server tests passed. Additional actual-app-handler tests exercise success/failure/diagnostic statuses in a DOM harness, not a real browser or hardware. Legacy scripts/test-host.sh was attempted and correctly rejected historical gui.c identity; it was not repinned. Full retention runtime suite not rerun here.

Managed browser localhost access was blocked as private/internal address; actual browser success/failure walkthrough remains for parent review. No all-files semantic review claim: original inventory remains 81 current/20 historical; this change reviews finding-related prose and targeted app handlers, not all remaining source/UI states, external first-party URL health or every anchor. Those gaps remain open and block a claim of full coverage.

## Exact remote payload drafts — DO NOT APPLY

Repository PATCH /repos/Jdelg718/entropylab-esp32p4:

```json
{"description":"Unofficial experimental ESP32-P4 educational mnemonic and fingerprint calculator. Public practice only; not a wallet or signer."}
```

Proposed new release payload (only after independent review and assignment of an approved new tag/commit; never replace v0.1.0-retention-preview):

```json
{"tag_name":"v0.1.1-retention-preview","name":"v0.1.1-retention-preview — documentation and onboarding","draft":true,"prerelease":true,"body":"Documentation/onboarding patch only. Firmware tuple and accepted source archive are unchanged from v0.1.0-retention-preview. Successful first-install completion now directs manual RESET and boot/display/touch/D6 checks rather than reinstallation. The public install policy remains hardware-unexercised; the separate retention updater remains held. Readback is not boot or recovery qualification. Verify the separately attached new package checksum; historical release assets remain unchanged. See docs/CURRENT-STATUS.md and docs/PUBLIC-FIRST-INSTALL.md for scope and exact firmware pins."}
```

This is a payload draft, not tag approval or a remote action. Package hash and inventory are reported by local execution to parent; publication requires independent review and honest closure of remaining gaps.
