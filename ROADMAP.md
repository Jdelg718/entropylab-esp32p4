# Roadmap and acceptance gates

## Product direction and assurance boundaries

EntropyLab aims to be an **offline seed/mnemonic + fingerprint generator on
supported general-purpose hardware**, with entropy-process assurance through
traceable input handling, explicit source assumptions and independently checked
conversion/derivation. It is **not a transaction or signing tool**. The first
BIP84 address is a derivation cross-check, not a transaction workflow.

Public-test-only use describes current test maturity, not a permanent product
limitation. Real-secret use needs an explicit threat-model and readiness review;
this roadmap does not authorize it. Statistical tests cannot prove randomness,
unpredictability or physical entropy provenance, and hashing cannot create
missing entropy. Offline application operation is not proof of a physical air
gap: the board's C6 presence must remain explicit in hardware threat modeling.

## Feature sequence

1. **Public fixture (historical):** compiled BIP39/BIP32/BIP84 computation,
   LVGL touch trigger, bounded C ABI and recorded hardware evidence. Historical
   source-image observations are not clean-checkout runtime verification.
2. **HEX + Coins (implemented):** native LVGL input and 12–24 BIP39 words,
   empty passphrase only, fingerprint and first mainnet BIP84 address. Coins
   maps raw Heads = 0 / Tails = 1 bits directly, without hash conditioning.
   HEX has recorded source-image hardware observations. Coins has exactly two
   recorded public-zero hardware presets (12 words / 128 bits and 24 words /
   256 bits); manual flips, Undo and mode isolation remain host-tested only.
   See [HEX milestone](docs/HEX-MILESTONE.md) and
   [COIN milestone](docs/COIN-MILESTONE.md) for source/build identities and actual
   verification limits. A newly built repository binary is not thereby
   hardware-verified.
3. **Hashed D6 (local candidate, review gated):** original COLDCARD-style raw
   digits and Coleman 6-to-0 before SHA256 remain distinct labeled modes with
   separate transcripts. Strict ASCII 1–6, maximum 1024 rolls, all five word
   counts. Exactly two short weak public source-image hardware tests were
   reported; threshold, soak and readiness hardware coverage is not claimed.
   See [DICE milestone](docs/DICE-MILESTONE.md). Hash output width and nominal
   roll thresholds do not certify entropy.
4. **Reliability and readiness (ongoing gates):** queue saturation, startup and
   allocation failure injection, cancellation/reset semantics, soak tests,
   stack/RAM headroom, secret-lifetime review and dependency review. Each feature
   requires bounded input contracts, host tests and separately recorded hardware
   tests; real-secret readiness requires a separate decision.
5. **Other upstream features (inventory pending):** trace the pinned upstream
   source and record implemented/partial/missing features, exact semantics,
   provenance and verification dependencies before choosing subsequent work.
   This is an inventory task, not a promise of untraced parity. Passphrase input
   and broader derivation paths remain unimplemented future scope.
6. **Optional microSD seed export (deferred design gate):** future export must be
   an explicit user action only, never automatic seed storage, background saves
   or implicit persistence. Before approval, decide the export format,
   encryption and key/passphrase handling, interoperability with named consumers,
   and a read-back verification procedure with failure/interruption behavior.
   A plaintext card export is an additional secret copy; deleting its file is
   **not secure erasure** of flash media. Warnings, secret lifetime and card
   handling belong in the threat model. **No export implementation or SD writes
   are authorized now.** This backlog item does not block other feature work.

7. **EntropyLab logo screensaver (deferred design gate):** use the authentic
   upstream website logo, not a recreation. Exact website asset provenance and
   redistribution/license review are pending; bundle the approved asset locally
   for offline use, with no runtime website fetch. Idle timeout and wake behavior
   require future design review. Do not overlay a secrets screen or imply that
   displaying the logo automatically erases secrets. No implementation is
   authorized by this roadmap item.

## Change and release gates

Signing and transaction workflows are outside the product direction. Real-secret
handling, storage/export, RNG and networking additions require a new threat model
and explicit decision, not incremental UI additions. Current application paths
remain without RNG, signing, storage or networking. Listing future export here
does not grant implementation approval.

Public release requires independent content/privacy/license audit and explicit
publication approval. Contributions are via reviewed changes, not automatic
access. This local candidate includes code; commits and pushes
remain gated on review and approval.
