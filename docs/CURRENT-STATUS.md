# Current release status

[v0.1.0-retention-preview](https://github.com/Jdelg718/entropylab-esp32p4/releases/tag/v0.1.0-retention-preview) was published on 2026-09-19. Start with [PUBLIC-FIRST-INSTALL.md](PUBLIC-FIRST-INSTALL.md) for downloads, local serving, backup/export, exact hardware and firmware hashes.

This unofficial native educational mnemonic and fingerprint calculator is for public practice only, not a wallet or signer. No upstream or hardware-vendor endorsement is implied. Native source includes optional BIP39 passphrase, LifeHash visualization, Cards, Bases, BitBox-style dice and D++ alongside Hex, Coins, D6, Words and Seed. Feature implementation does not establish entropy quality, broad hardware qualification or real-secret readiness. See the [method table](../README.md#what-is-implemented).

The exact three-image tuple previously received private-path browser write/readback and human boot/display/touch/D6 Back-retention acceptance on one board. The subsequent unpublished speed/progress runtime `dc5c654` received human-operated public-policy installation acceptance on EL-002: the browser displayed all six image/tail hashes verified, the board booted, and the user confirmed 12/24-word results and Back/reopen retention. This is one-board observed acceptance, not raw physical baud/timing telemetry; no repeat flash is needed for that milestone. See [physical acceptance](../evidence/physical-acceptance.md) and [qualification report](../GATES-REPORT.md). Independent native full outside-window comparison did not finish: **NOT PASS**. No outside-window equality, preboot continuity, tested restoration, broad application qualification or reproducible firmware-build claim follows. The separate retention updater remains HOLD.

Historical accepted-source host gate: `python3 scripts/retention-host.py` ([prerequisites and identity checks](retention-host/README.md)). The integrated education source intentionally fails its accepted-checkout identity check; the archived runner cannot certify successor lessons. Live education uses `EDUCATION_ONLY=1 bash scripts/test-gui-host.sh` with native prerequisites. Historical documents retain their original source-bound evidence; their old “current” labels are not current release status.

## Unpublished candidate readiness

The speed/progress implementation is no longer paused. Native macOS/Windows synthetic-serial suites and the prior exact candidate distribution passed; this does not qualify other OS physical drivers or imply publication. The published preview remains distinct from this unpublished candidate. [Release readiness](RELEASE-READINESS.md) separates required final-package review/authorization from gates for broader claims. This local successor now integrates education copy, readiness documentation and event-driven browser animation. The accepted binaries/archive are unchanged and do not contain the new education lessons; no target build or installation of those lessons is claimed. Browser/native-host checks do not extend the historical hardware acceptance. A fresh official package is deferred until a reviewed successor source/binary provenance contract can represent this boundary without bypassing accepted-source checks.

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

## Provenance context

The accepted archive and firmware tuple are unchanged; [source pins](PUBLIC-FIRST-INSTALL.md#sources-and-pins) remain authoritative. Publication commit `423702a7746cc9d301aeb9459172c6988992d007`, reviewed source candidate `06cee81a764b1725a9c63272734d2259c76cedfb`, accepted archive and firmware identities are distinct.

The source-local LifeHash `ADAPTATION.md` records revision-05, not the complete later portability history. Later `grid.hpp` and `hex.cpp` edits are represented in its current `VENDOR-SHA256.txt` and combined/retention source mappings; the tree is not pristine upstream. The original adaptation record is inside accepted identity and remains byte-for-byte intact. See [combined mapping](MODAL-COMBINED-SOURCE-MAPPING.json) and [retention identity recipe](retention-host/README.md).

Embedded relative references in concatenated browser NOTICES use repository root, not the installer directory. Consult [root THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). License and concatenated notice bytes are unchanged.

## Published checksum layout

Verify the published `public-release.zip` entry before extraction. The other published SHA256SUMS entries, `inventory.json` and `retention-public-source.tar.gz`, refer to files under `release/` after extraction. Do not run the whole published checksum list against the extraction root without accounting for that layout. Published tags/assets remain immutable; any documentation patch needs a separately reviewed new package.

## Historical document routing

Accepted source includes pinned documentation as well as code. Exact originals remain immutable inside the accepted source archive and Git history. The retained unpublished v0.1.1 documentation/onboarding work prepends current-guidance links directly to 21 public root/docs files, including GitHub-discovered CONTRIBUTING and SECURITY. The finite [documentation successor map](retention-host/documentation-successors.json) pins each exact old/new hash; the checkout validator checks that pinned map without changing accepted archive, target identity, historical scripts or firmware pins. In the documentation-only predecessor, only those exact documented checkout prose successors differed and archived identity437 remained exact. This integrated successor additionally changes education firmware source outside that finite documentation mapping, so accepted-checkout identity is intentionally rejected; archived source and its pins remain untouched. Corrected current companions below contextualize the historical contents.

- [CHANGELOG.md](current/CHANGELOG.md)
- [CONTRIBUTING.md](current/CONTRIBUTING.md)
- [ROADMAP.md](current/ROADMAP.md)
- [SECURITY.md](current/SECURITY.md)
- [docs/BUILD-FLASH.md](current/docs/BUILD-FLASH.md)
- [docs/BUILD.md](current/docs/BUILD.md)
- [docs/COIN-MILESTONE.md](current/docs/COIN-MILESTONE.md)
- [docs/DICE-MILESTONE.md](current/docs/DICE-MILESTONE.md)
- [docs/EDUCATION-PROVENANCE.md](current/docs/EDUCATION-PROVENANCE.md)
- [docs/HANDOFF.md](current/docs/HANDOFF.md)
- [docs/HARDWARE.md](current/docs/HARDWARE.md)
- [docs/HEX-MILESTONE.md](current/docs/HEX-MILESTONE.md)
- [docs/INPUT-EXPLANATIONS.md](current/docs/INPUT-EXPLANATIONS.md)
- [docs/KNOWN-LIMITS.md](current/docs/KNOWN-LIMITS.md)
- [docs/MODAL-COMBINED-STATUS.md](current/docs/MODAL-COMBINED-STATUS.md)
- [docs/PROVENANCE.md](current/docs/PROVENANCE.md)
- [docs/PUBLICATION-STATUS.md](current/docs/PUBLICATION-STATUS.md)
- [docs/RELEASE-BUILD-HYGIENE.md](current/docs/RELEASE-BUILD-HYGIENE.md)
- [docs/UPSTREAM-FEATURES.md](current/docs/UPSTREAM-FEATURES.md)
- [docs/WORDS-MILESTONE.md](current/docs/WORDS-MILESTONE.md)
- [docs/d6-release/README.md](current/docs/d6-release/README.md)
