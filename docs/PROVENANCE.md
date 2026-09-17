# Review-repair successor — current provenance boundary

The complete text below is preserved historical predecessor evidence, including its old “current” labels. This successor changes the GUI index-format buffer, idle-saver brand text and its host assertion. Use [REVIEW-REPAIR-SOURCE-MAPPING.json](REVIEW-REPAIR-SOURCE-MAPPING.json) and [review-repair-successors.json](review-repair-successors.json) for current source lineage. No complete target image, installed-image identity or publication approval is asserted.

---

# Current D6 source candidate — provenance boundary

The release-identity section below is preserved HISTORICAL predecessor evidence, not a source-to-device mapping for this candidate. Current production GUI/navigation and host tests come from the frozen D6 successor listed in [RELEASE-SOURCE-MAPPING.json](RELEASE-SOURCE-MAPPING.json). No current installed-image or target hash is asserted. All upstream/dependency attribution and license terms below are preserved verbatim.

---

# Provenance and release identity

This tree is the frozen public-source packaging candidate for the installed education/UI application. It is not yet publication-approved, committed, pushed, merged, tagged, or released.

## Exact release chain

- Accepted production source manifest: `52b03b938e6f8aa172bb1219c851f77de3494cfea14e5ac28a78785960de1b35` (402 entries).
- Target application image: `f0c4edf1267470991d4750de027604c2e512ace943223ce42ca6c57b246afebe`.
- Installed application readback: the same SHA-256 as the target image.
- Installation evidence records verified boot plus unchanged bootloader and partition bytes.
- The exact chain and receipt hashes are in `docs/INSTALLED-SOURCE-MAPPING.json`.
- No reproducible-binary claim is made. The mapping relies on independently recorded source/build/install/readback identities.

The included host tests contain one independently approved successor correction in `fixture-firmware/tests/modal_touch_tests.inc`. All 401 other frozen entries, including every production source entry, are byte-identical to the accepted production source. See `docs/COMBINED-SOURCE-MAPPING.json`.

## Upstream and dependency provenance

- EntropyLab upstream: https://github.com/OogaBoogaX/entropylab at `6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51`. Extracted/adapted material retains the exact custom Ooga Booga license where applicable.
- Waveshare baseline reference: https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3 at `dc1b2085381c9cc3a2badf8d317673cca9079c1f`. No vendor tree is bundled.
- ESP-IDF reference: v5.5.5 at `b774170ff46c393eeb5e495ea37936038d3f4f4f`.
- Exact managed-component resolution is retained in `fixture-firmware/app/dependencies.lock`; registry/vendor caches are excluded.
- Public BIP32/BIP84/BIP39 snapshots and derived vectors are hash-pinned in `core-spike/vectors/sha256.json` and covered by retained BSD-2-Clause, public-domain, and MIT notices.
- bc-lifehash source retains BSD-2-Clause-Patent and bundled SHA notices under its source-local notice directories.
- Fonts retain adjacent SIL OFL and Liberation notices; the dictionary retains its MIT notice.
- Logo/artwork source and transformations are recorded adjacent to the assets. No separate trademark grant, affiliation, or endorsement is claimed.

The source distribution is mixed-license, not blanket MIT. `THIRD_PARTY_NOTICES.md`, source-local provenance files, full adjacent license texts, and `docs/COMBINED-RUST-LICENSES.json` are authoritative. This package is not a legal opinion and does not claim complete binary-distribution clearance.

## Education provenance

Method-by-method source, behavior references, licensing, deterministic public examples, no-endorsement statements, and exact review receipt hashes are recorded in `docs/EDUCATION-PROVENANCE.md`.

Private logs, absolute private paths, network addresses, device identifiers, credentials, real seeds, caches, build outputs, binaries, archives, maps, and device evidence are excluded from the public tree.
