# Words and practice-cleanup candidate

## Source scope

This local candidate follows the published Dice base
`8e5e721c31e7d2aa09e093e7b4dd8d5bc39db1de`; it is not yet a published release.
It incorporates authenticated installed application source for:

- English BIP39 Words entry/validation at 12/15/18/21/24 words, prefix selection
  and a taller editor. Count, dictionary and checksum validation precede derivation.
- Empty-passphrase BIP32 master fingerprint and first mainnet BIP84 address;
  no passphrase-entry, signing or wallet-management feature is implied.
- Cross-mode removal of repetitive general banners, with consistent Safety/About
  and credits. Result-specific no-funds, checksum-not-quality, invalid/weak-input
  and method warnings remain.
- Practical selected-mode Clear: best-effort application-owned input/result cleanup.
  This excludes compiler temporaries, cryptographic internals and old allocations;
  it does not wipe all RAM or certify secure erasure.
- Authentic Orbit artwork and Words-only saver after 60 seconds of **eligible**
  inactivity. Concealment preserves committed Words. The first wake contact is
  consumed rather than activating the underlying control; this is not a lock/wipe.

`fixture-firmware/runtime` builds a unified runtime from the four copied cores in
`runtime-sources`. Its effective Cargo graph is separate from historical standalone
locks. Target and GUI builds compile fresh source, not a retained historical archive.
The bounded host upstream extraction remains a different implementation.

## Evidence boundaries

| Layer | Candidate status |
| --- | --- |
| Source | Words, editor, saver and practical cleanup imported; independent code/security and publication content/privacy/license reviews passed |
| Host | Full public-fixture suite passed, including debug/release Rust tests, C worker tests, provenance negative tests and GUI against pinned LVGL 9.5.0 |
| Target | Published build script passed from fresh runtime source; revision, ABI, API ownership and component-lock gates passed |
| Device | Candidate not flashed; no new physical acceptance claimed |

The earlier separately installed practice-cleanup image has historical exact app
readback and boot verification. That is not the identity or runtime evidence of a
new publication build. Its current physical UI/touch/computation acceptance also
remains pending. Earlier user observations do not transfer to changed images.
Host tests do not establish contact behavior, target RAM/stack headroom, timing,
reproducibility, or funded-wallet readiness.

Remaining physical checks use public fixtures: scrolling, eligible idle, wake over
controls, held/overlapping contacts and the normal next contact. No private seed
re-entry, recovery or forensic-memory work is requested. Words has no numeric
randomness score; checksum validity and encoded width do not prove entropy quality.

## Attribution and deferred scope

Credit **EntropyLab — Team Ooga Booga** and the
[upstream project](https://github.com/OogaBoogaX/entropylab), with Mr.Hodl’s public
[origin account](https://x.com/mrHodl/status/2099170677245014304) and
[calculator scope](https://x.com/mrHodl/status/2099506569931010421), not sole authorship
or endorsement. See [provenance](PROVENANCE.md) and
[third-party notices](../THIRD_PARTY_NOTICES.md).
Fairness, passphrase entry, LifeHash, education, SD export and developer-mode
proposals remain deferred under the [roadmap](../ROADMAP.md), not implemented.
