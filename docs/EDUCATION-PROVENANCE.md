> **Current guidance:** [EDUCATION-PROVENANCE — current retention preview](current/docs/EDUCATION-PROVENANCE.md).
> The text below is the historical source checkpoint, not current release status.
> v0.1.0-retention-preview is published; v0.1.1 is a local documentation/onboarding patch pending publication.
> Firmware is unchanged; public-policy hardware qualification and the separate retention updater remain held.

# Education content provenance

Status: frozen source-package candidate; independent publication review pending.

The practical education text is stored in `fixture-firmware/app/main/education_content.inc` (SHA-256 `0db37c218825e778b2c22aaf178128afc127ed720b99ffe754b89f9301065cf4`). The release text was prepared for this project and is covered as project-authored wrapper/documentation material under `LICENSE-MIT`. No third-party instructional prose is represented as copied or endorsed. Algorithm and implementation provenance remains mixed-license and is governed by `THIRD_PARTY_NOTICES.md`, adjacent license files, source-local provenance, and retained upstream snapshots.

All examples are deterministic public practice fixtures marked `PUBLIC PRACTICE - NEVER FUND`. They are not private inputs, do not demonstrate secure generation, and must not be funded. No upstream author, project, or vendor endorses this adaptation. Logo inclusion has source-local provenance and retained custom license coverage; no separate trademark grant is claimed.

Exact review identity for all methods:

- production source manifest: `52b03b938e6f8aa172bb1219c851f77de3494cfea14e5ac28a78785960de1b35`
- bounded visual receipt: `ad391be8c07e020beb3a9f85b59e7376ebce2d1845ff86e88686d9787c2e5f9e`
- technical SPEC receipt: `20290501628a47437563b85d3e3252d718ba9367b3309e96061324ee6a009c03`
- distinct SECURITY receipt: `639b707cc317ab6969bb8e89028ca2f2447fd09d5bc92c4787a1a620766606e6`

## Method records

### Words and Seed word numbers

- Education source: `fixture-firmware/app/main/education_content.inc`.
- Behavior sources: `fixture-firmware/app/main/mnemonic_editor.inc`, `mnemonic_gui.inc`, `seed_native.c`, and the pinned BIP39 dictionary.
- Provenance: BIP39 public vectors and dictionary origins are recorded in `docs/VECTOR-LICENSE-SOURCES.md`, `docs/words-import-manifest.json`, `LICENSE-TREZOR-MIT`, and `fixture-firmware/LICENSE-dictionary.txt`.
- Teaching boundary: these routes import existing entropy. Typing, lookup, checksum filtering, hashing, PBKDF2, and LifeHash do not add entropy.

### Cards

- Education source: `fixture-firmware/app/main/education_content.inc`.
- Behavior sources: `fixture-firmware/app/main/gui08_editor.c` and `gui08_native.c`.
- Provenance: project-authored education text under MIT; implementation and dependency licenses remain as noticed.
- Teaching boundary: direct rank phases are supported; suits and deck-permutation algorithms are not represented as implemented.

### Bases

- Education source: `fixture-firmware/app/main/education_content.inc`.
- Behavior sources: `fixture-firmware/integrated-lanes/bases/core/number_bases.c`, `gui08_editor.c`, and `gui08_native.c`.
- Provenance: project-authored education text under MIT; retained implementation notices apply.
- Teaching boundary: base 4/8/32/64 is deterministic encoding of externally produced bits, not an entropy source.

### BitBox-style dice entry

- Education source: `fixture-firmware/app/main/education_content.inc`.
- Behavior sources: `fixture-firmware/input-methods/extra-dice/extra_dice.c` and `fixture-firmware/app/main/extra_dice_native.c`.
- Provenance: project-authored explanatory adaptation under MIT; no BitBox affiliation or endorsement is claimed.
- Teaching boundary: five D4 outcomes are made with D6 rejection of physical 5/6, followed by an independent coin-like sixth outcome; deterministic grouping/checksum handling adds no entropy.

### D++

- Education source: `fixture-firmware/app/main/education_content.inc`.
- Behavior sources: `fixture-firmware/input-methods/extra-dice/extra_dice.c` and `fixture-firmware/app/main/extra_dice_native.c`.
- Provenance: project-authored explanatory adaptation under MIT; no independently established third-party D++ standard, affiliation, or endorsement is claimed.
- Teaching boundary: the 18-word final sequence is D16 then D8; D8 faces 1-4 map to bit 0 and 5-8 map to bit 1, with no discarded final D8 face.

## Test-only successor

`fixture-firmware/tests/modal_touch_tests.inc` contains the independently reviewed host-test-only stale-marker correction. The test-package manifest is `a3c90b2c05297263a4e1aa06f554d8ed39891bc3f52fdb210f602d1ed307e17d`. It differs from the production source manifest at exactly that one test fixture; all 401 other entries, including every production input, are byte-identical. This does not alter or relabel the installed image.
