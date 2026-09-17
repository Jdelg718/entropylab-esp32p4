> **Current combined successor — publication HOLD.** This tree combines the exact review-repair package with the independently host-reviewed two-file modal repair. Combined full host, default+10 GUI, target build and device gates have **not been run** on this tree. Individual predecessor PASS evidence is not combined execution evidence. Current identity suites pass; see `docs/MODAL-COMBINED-STATUS.md` and `docs/MODAL-COMBINED-SOURCE-MAPPING.json`. Statements below describing the prior modal failure or review-repair-only identities are preserved historical predecessor evidence, not current-tree results. Historical manifests/mappings are not rebaselined.

# Release priority — current candidate

1. Independent source/content/license review of the exact candidate.
2. Complete clean-checkout host/CI pass and sole integration target identity/resource gate.
3. Public-fixture physical D6 and regression acceptance.
4. Validate complete first-install kit separately from compatible app-only updates; publish immutable source-bound checksums and preserve tested recovery.
5. Reviewed board-selecting browser flasher for the exact Waveshare model, with fail-closed compatibility checks, explicit overwrite consent, pinned dependencies and manual fallback. No promised automatic board detection or broad browser support.

Unrelated feature expansion is not a release prerequisite. Diagnostics expansion, new wallet tools, additional hardware/layout targets and secure-storage/signing features remain separate work. No timeline or shipped functionality is implied by the historical roadmap below.

---

# Roadmap and acceptance gates

## Product direction

An unofficial offline seed/mnemonic and fingerprint generator/checker for supported
general-purpose hardware, with entropy-process assurance and education. The default
product is not a transaction signer. Statistical tests, encoded width, checksum
validity and hashing do not establish source randomness. Offline app operation does
not prove a physical air gap; C6 remains a hardware threat-model concern. Public-test
use is the present maturity boundary, not the permanent product goal.

## Future screens and devices — planned, not supported today

The only hardware profile historically exercised with predecessor firmware is the Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3,
480 × 800, on the accepted Rev1.x profile. Other screen sizes, orientations,
panels, touch controllers, ESP32-P4 revisions, and hardware devices are future work.
There is no promised date or compatibility matrix.

Contributions are welcome for portable layout metrics, display/touch adapter
boundaries, board profiles, public-fixture host tests, target-build checks, and
hardware test plans. Compilation or a mockup alone does not establish support. Each
new target needs exact hardware identification, source review, pinned dependencies,
target build, and separately authorized physical acceptance before it can move from
experimental to supported.

## Current checkpoint

This candidate imports Words prefix selection, a taller editor, practical Clear,
common Safety/About and the authentic Orbit saver atop published HEX/Coins/Dice.
The separately installed practice-cleanup source image has recorded app readback
and boot verification; **its current physical UI/touch/computation acceptance is
pending**. A new publication build has a distinct identity; historical observations
do not establish its behavior. See [handoff](docs/HANDOFF.md).

1. Retain completed cross-mode banner cleanup, Safety/About credits and practical
   Clear. Preserve result-specific no-funds, checksum-not-quality, invalid/weak-input
   and method warnings. Clear affects selected-mode owned inputs/results, not all
   SRAM/PSRAM, compiler temporaries, crypto internals or old allocations.
2. Separately close physical public-fixture scrolling, eligible Words idle,
   wake-over-actions, held/overlapping contacts and next-contact checks. Measure
   resource/performance budgets separately; host checks are not physical evidence.
3. [Compact input explanations](docs/INPUT-EXPLANATIONS.md) are the next bounded
   native candidate: encoded width versus source randomness, raw Coins mapping,
   distinct D6 hashing transcripts, all-roll inclusion and fair-independent-source
   assumptions. Consult its verification status before treating it as shipped.
   Words has no numeric randomness score. No seed recovery or forensic memory work.
   Orbit begins after 60 seconds of eligible Words inactivity and consumes the
   first wake contact; it preserves input and is not a lock or wipe.
4. Optional separately reviewed memory-only fairness diagnostics and explicit
   passphrase input require sample limits, canonicalization/length contracts,
   independent vectors and clear-state handling. LifeHash alongside text and
   bounded education are later work, not cleanup blockers.

## Retained future proposals — not implementation approval

- Reliability: startup/allocation/queue failure injection, reset semantics, soak,
  stack/RAM headroom and secret-lifetime/dependency review. Real-secret use requires
  an explicit readiness decision and threat model.
- [Pinned upstream feature map](docs/UPSTREAM-FEATURES.md): records present native
  coverage and the next candidate features. Reinspect official source before each
  implementation; broader paths remain deferred.
- Optional microSD export: explicit user action only; never automatic storage.
  Review format, encryption/key handling, named-consumer interoperability,
  readback and failure/interruption behavior first. Deleting a plaintext export
  is not flash secure erasure. No export implementation or SD writes now.
- Educational videos/lessons: locally bundled, offline, public fixtures only;
  verify entropy/checksum/seed-space claims independently. Gate source/media rights,
  board playback/resources, readable captions and touch controls. No automatic
  secret writes or required network connection.
- Future opt-in developer mode: separately gated wallet/signing or microSD
  interoperability; default generator operation remains isolated. Enabling a mode
  must not itself save/export secrets, enable networking or sign. Require explicit
  activation, visible status, acknowledgement and cleanup. Bitcoin Core/Sparrow
  integration must distinguish mnemonic backups, descriptors/xpubs and PSBTs;
  do not assume arbitrary mnemonic files are accepted. Signing requires reviewed
  key lifecycle, transaction parsing, trustworthy confirmation and independent
  public-fixture/test-network tests before any funded-wallet decision.
- Transactions, PSBT, vanity, signing, storage/export and C6 hardening remain
  separate deferred scopes, not prerequisites silently added to practice cleanup.

## Credits, assets and release gates

Credit **EntropyLab — Team Ooga Booga**,
[upstream](https://github.com/OogaBoogaX/entropylab), and Mr.Hodl's publicly
reported [origin](https://x.com/mrHodl/status/2099170677245014304) and
[calculator scope](https://x.com/mrHodl/status/2099506569931010421), without sole
ownership/authorship or endorsement claims. Preserve exact custom Ooga Booga
license and third-party/font notices. Authentic logo assets retain pinned
provenance; repository-wide license coverage is an interpretation supported by
upstream distribution and README, not a separate trademark grant or legal certainty.

Source publication requires independent content/privacy/license review. Host,
target build, installed identity and physical acceptance are separate gates.
Future proposals grant no permission for hardware operations, secret handling,
networking, RNG, persistence or signing additions.
