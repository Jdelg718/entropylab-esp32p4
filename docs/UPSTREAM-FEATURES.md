# Native scope and next upstream features

This is a bounded feature map against [official EntropyLab source at
`6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51`](https://github.com/OogaBoogaX/entropylab/tree/6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51),
not a claim of parity with its latest website. Upstream builds a self-contained
HTML application; this repository selectively implements native ESP32-P4 flows.
Feature availability is not real-secret readiness.

## Present native feature set

- HEX and raw Coins/binary input; five BIP39 word counts.
- Distinct raw and Coleman 6-to-0 hashed D6 transcripts.
- English Words entry, prefix suggestions, count/dictionary/checksum validation.
- Master fingerprint and first mainnet BIP84 address, empty passphrase only.
- Independent mode inputs, practical selected-mode Clear, Safety/About, and
  Words-only Orbit idle/wake behavior.

See [Words milestone](WORDS-MILESTONE.md) for verification boundaries and
[roadmap](../ROADMAP.md) for the approval gates. A published branch is not a
firmware installation or hardware acceptance result.

## Recommended next sequence

| Feature | Native status | Upstream reference and next boundary |
| --- | --- | --- |
| Entropy-method explanations / fairness diagnostics | Counts and weak-input warnings exist; diagnostic panels absent | `src/js/app.js:2631–2845,4664–4730`. Explain encoded width versus randomness first; optional bounded memory-only statistics must not certify entropy. |
| BIP39 passphrase | Empty only | `src/js/bip39.js:23–25,85–96`; `src/js/app.js:3720–3780`. Design exact input/normalization/length and clear-state contracts; preserve spaces/case, independently check vectors. |
| LifeHash alongside fingerprint | Absent | `src/js/lifehash.js`; README feature list. Identity visualization, not an entropy-quality score, authentication guarantee, or substitute for text. |
| Additional entropy methods | Absent beyond HEX/Coins/hashed D6 | `src/js/app.js:156–159,2413–2630,3238–3505,4572–4624`. BitBox diceware, D++ D8/D16 and card methods require separate exact mappings; do not treat them as aliases of hashed D6. |
| Broader derivation and address verification | Fixed BIP84 address only | `src/js/app.js:249–361,803–942,1069–1288`. BIP44/49/84/86/48, networks, paths and address tables require explicit bounded scope. |
| Offline educational lessons | Roadmap proposal | Native education is its own UI/media/resource milestone, not a claim that every lesson exists upstream. Explain entropy, checksum, passphrase and derivation using public examples. |

Close remaining finite physical public-fixture checks separately: scrolling,
eligible idle, wake-over-controls, overlapping contacts and ordinary next touch.
Measure target stack/RAM/performance before making resource-readiness claims.
These checks must not silently turn source-publication cleanup into a new feature project.

## Deliberately separate future scopes

The pinned HTML also has broader workspace, multisig, BIP85, QR/export, key,
transaction/PSBT and other advanced tools. Presence upstream is not authorization
to copy them into this device. Signing/transactions, private export/persistence,
vanity, Lightning and Silent Payments remain outside the current native milestone.
BIP39 passphrase entry must not bypass validation or feed unnormalized Unicode
into a normalized-only derivation API. No recovery of user seed material is needed
for any roadmap work.

Code/source links above are anchored to the pinned revision. Reinspect upstream
before implementing the next selected feature; upstream may have changed.
