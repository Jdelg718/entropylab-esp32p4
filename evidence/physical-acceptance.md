# One-board physical acceptance — 2026-09-20

Runtime: `dc5c65420aa96013ae3847ab4771a7f520f1c3a8`; automated qualification evidence: `56e71216f184915f1198878cc1b3d46829044321`. Accepted firmware unchanged. Device lab label: EL-002.

## Method and evidence

Controller rechecked reviewed staged runtime hashes and served installer assets before the human-operated native-browser run. Device permission was granted to the existing operator account. No competing accessible serial holder was observed before the run.

1. User screenshot showed port selected, not opened.
2. Diagnostic screenshot showed SECURITY_CHECKS_CLEAR, ROM ECO 2, secure boot false, secure download false, flash encryption count zero, and diagnostic completion without firmware writes.
3. User was instructed to reload, verify assets, reselect the same device and perform one explicit installation.
4. Subsequent browser screenshot showed completion: all three images and erased tails verified. This is observed browser-result evidence, not an independently captured raw serial trace.
5. Following reset instructions, user supplied physical board photographs showing booted 12-word and 24-word result screens, fingerprint/LifeHash/address output. User reported both results matched the prior test.
6. Asked specifically whether Back followed by reopening Test results without recalculation preserved the same result, user confirmed: "yes all good".

## Acceptance

One-board user-operated installation, displayed image/tail verification, boot, interactive result checking, and Back/result retention are accepted. No additional flash is needed for this milestone.

This run did not capture physical baud timing or independently recompute the photographed results. Recovery/restoration, outside-window flash comparison, other-OS physical serial support and spoken screen-reader usability remain separate qualification tasks. No publication occurred. Photographs and mnemonic/address contents are not included in this evidence record.

The earlier automated operator's zero-write failure receipt remains valid for that earlier attempt; this later human-operated completion supersedes its unresolved basic installation handoff.
