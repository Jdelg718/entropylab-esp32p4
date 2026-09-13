# Vector license source verification

The original retrieval commits were not recorded. These URLs were fetched for
license verification; their exact SHA-256 digests identify the bytes examined,
not an inferred historical revision. No vector snapshots were changed.

| Source URL | SHA-256 | Use |
| --- | --- | --- |
| https://raw.githubusercontent.com/trezor/python-mnemonic/master/LICENSE | `d5e3c7c62a84e80073201e2f6e5130e9e6804fa05f8ac4f8b26a13c7d3969697` | Byte-identical LICENSE-TREZOR-MIT |
| https://raw.githubusercontent.com/trezor/python-mnemonic/master/vectors.json | `fa3b937b7cff9c9b8ecd3aa011faeb8d6dd67993174b72326e83f4de8fdb30f8` | Matches bundled bip39.json |
| https://raw.githubusercontent.com/bitcoin/bips/master/bip-0032.mediawiki | `e5e00a8289db2f681052cf24a745320afc225e66b25d1e489a7c884d2fc7f11f` | Matches bundled BIP32; author and license grant |
| https://raw.githubusercontent.com/spdx/license-list-data/main/text/BSD-2-Clause.txt | `f32fb3b417a194167cfad068223fc975ba96c5960513a10f66a3c28720aec1df` | Standard full BSD-2-Clause conditions/disclaimer |

LICENSE-BIP32-BSD-2-CLAUSE preserves the BIP's author and grant, and reproduces
all SPDX conditions and the disclaimer verbatim. The SPDX placeholder copyright
line is omitted rather than inventing a year or presenting a template as an
upstream copyright notice. The BIP's original embedded notices remain intact.

See [third-party mapping](../THIRD_PARTY_NOTICES.md) for the documents and derived
TSV files covered by each license. Existing vector hashes remain in
`core-spike/vectors/sha256.json`.
