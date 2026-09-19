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

Independent follow-up at ae954d6931039ec9387d74c86cef5a7bdfa0fd9c: GitHub-discovered root CONTRIBUTING/SECURITY/ROADMAP and 18 other public documents now prominently start with current-guidance links and explicit historical framing. Their exact original body bytes remain intact below the banner and inside the immutable accepted archive/Git history. The pinned documentation-successors.json is a finite 21-path old/new SHA-256 mapping. retention-host.py validates both predecessor archive bytes and exact successor checkout bytes; no accepted archive, identity437, historical mapping or original verifier pin was changed. Mutation tests reject nonallowlisted changes and mapping additions. Items 2–5, 8–10 and 16–17 above therefore now have direct original-path discovery, not companion-only discovery. README explicitly distinguishes the unpublished v0.1.1 patch from published immutable v0.1.0.

## Verification and remaining gaps

Independent follow-up verification: current retention-host.py --check passes archive745/identity437 and historical436 with the exact finite prose successors; review-public.py passes notices282 and firmware fixture source330 byte equality. All 25 Node tests pass. Public server allow/deny and byte-serving checks pass. Local headless Chromium (Playwright, loopback server bound to port 0) loaded the actual served app and exercised success, failure and diagnostic handlers with an explicitly mocked transport: success shows manual RESET and no fresh-attempt reinstall instruction; failures never instruct RESET; diagnostics stay separate; board/consent controls gate writes and resetOnSuccess remains false. This is browser UI simulation, not hardware qualification. Legacy scripts/test-host.sh was rerun and still rejects its historical gui.c identity; no pins were refreshed to hide that expected predecessor mismatch. Full retention runtime suite was not rerun.

Current first-party relative Markdown paths and anchors pass an automated check (92 links across 24 documents: README, CURRENT-STATUS, PUBLIC-FIRST-INSTALL and all 21 current companions). The one distinct first-party external release link in those Markdown links was fetched successfully and identifies the published v0.1.0 prerelease. The regenerated ZIP passed extracted/no-Git allowlist tests, including denial of private sentinel files; two consecutive package runs produced the same SHA-256. This is not a full vendor legal-text review, exhaustive UI/state audit, permanent external URL availability guarantee or firmware/runtime qualification. Original audit coverage distinctions remain applicable.

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
