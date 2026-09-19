# Independent public installer review

Reviewed base `d7258eb37c70ffddb1970fdfec9fe65ea5682d9d` in an isolated clone.

**GO for the bounded public installer/security review after these fixes; publication remains held for the coordinator's final source binding.** No hardware, SSH, remote write or publication occurred.

## Fixed

- The documented generic Python server exposed Git metadata and untracked files. The new loopback server permits only tracked public files, rejects symlinks, traversal and directory listings. Eight allow/deny regressions pass.
- Historical README status was marked only in an invisible HTML comment; now a visible warning directs readers to the current contract.
- Added shipped-vendor wire regression coverage rather than relying solely on replacement-loader tests.

## Executed

- 22 Node tests pass, including 11 shipped-vendor wire cases: success, wrong chip/revision/ECO/security flags/encryption/density, write-block failure, corrupt tail, expired authority and cancellation. Vendor ESPLoader performs real compression, commands, security parsing, register revision decoding, stub protocol and readback transactions against a synthetic transport. Physical serial sync and JEDEC peripheral replies are modeled, not hardware-tested. A failed block is attempted once; failures do not reset; used sessions cannot run again.
- The same 11 wire cases passed inside headless Chromium using the shipped browser adapter/vendor. Browser harness substituted Web DecompressionStream for Node inflate and a 16-byte framing-only trailer. This is browser protocol simulation, not physical Web Serial acceptance.
- Chromium rendered the real installer and imported the real assets loader; all three fetched images passed actual SHA-256 verification. Board unchecked blocks port selection; board checked enables selection; write remains disabled without port and destructive consent. Documentation link resolves. Screenshot retained locally, excluded from package.
- Public policy has **six** post-write reads: three images plus three FF tails. The old private policy's nine included three predecessor reads; retaining those would contradict removal of private factory authority. No private predecessor hashes are in the public profile.
- Exact policy requires chip 18, silicon 100–199, ECO 0/2, known flags, secure boot/download off, zero encryption count and raw 32 MiB JEDEC density. Physical PCB rev1.3 remains a human assertion; new public policy is expressly not hardware accepted.
- Tracked source and generated ZIP scanned for private paths, tailnet addresses, credentials, identity indicators and ELF/map artifacts. No relevant hit: `kent` matches were upstream `jkent/frogfs` attribution and dictionary `token`; backup filename was intentional user documentation. Archive contents separately scanned. No ELF/map included.
- Accepted archive validator passes source745 / identity437 / delta5+1 / notices301. All 330 fixture source files equal the accepted archive bytes. All 282 manifest notice hashes match and exact notice bytes occur in installer notices.

## Boundaries

The inherited full host runner still fails `review-repair current mismatch: fixture-firmware/app/main/gui.c` before running suites, as disclosed in current docs. Not represented as a new passing host run. No firmware bytes or source changed. No Windows/macOS native execution, new hardware acceptance, whole-flash equality or reproducible firmware build claimed. Coordinator must regenerate and bind final inventory/ZIP after merging.
