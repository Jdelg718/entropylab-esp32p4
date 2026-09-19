# Third-party notices

This is a mixed-license source distribution; not a blanket MIT relicensing.

The combined candidate additionally contains adapted Blockchain Commons
bc-lifehash sources (BSD-2-Clause-Patent, with the separate SHA256 notices
preserved in its `binary-notices` directory). See that subtree's provenance and
the exact combined source mapping. Runtime registry dependencies remain fetched,
not vendored; supplemental license texts and declared license expressions for
the exact expanded graph are inventoried in `docs/COMBINED-RUST-LICENSES.json`.
Some published crates contain no standalone license file, explicitly shown by
an empty notices list. This is not complete binary-distribution clearance.

- `core-spike/native/src/upstream_core.rs`: OogaBoogaX/entropylab,
  commit 6e1f39cc7da25942c7a1f51ea5837f4ac7ef8f51, exact custom
  THE OOGA BOOGA LICENSE (LICENSE-OOGA-BOOGA), not the Unlicense.
- New wrappers/fixture/scripts/docs: LICENSE-MIT; original copyright retained.
- `fixture-firmware/app/main/fonts/el_*.c`: derived Liberation font software,
  SIL OFL 1.1, **not MIT**. Full upstream copyright/license and reproducible
  source/converter provenance are retained in that directory.
- `core-spike/vectors/bip39.json` and derived `bip39-english.tsv`:
  trezor/python-mnemonic, MIT; full upstream copyright and permission notice
  is reproduced byte-for-byte in [LICENSE-TREZOR-MIT](LICENSE-TREZOR-MIT),
  including Copyright (c) 2013-2016 Pavol Rusnak.
- `core-spike/vectors/bip-0032.mediawiki` and derived `bip32.tsv`:
  BIP32 by Pieter Wuille, BSD-2-Clause; full conditions and disclaimer are in
  [LICENSE-BIP32-BSD-2-CLAUSE](LICENSE-BIP32-BSD-2-CLAUSE).
  Original embedded attribution and license statement remain unchanged;
  no copyright year is inferred from the BIP assignment date.
- `core-spike/vectors/bip-0084.mediawiki` retains its embedded public-domain
  dedication. All bundled vectors are public fixtures, not private data.
- The duplicate BIP39 and BIP84 fixtures in `fixture-firmware/rust/vectors`
  are byte-identical to the sources above and covered by those same full notices.
  See docs/VECTOR-LICENSE-SOURCES.md for pinned source provenance.
- Imported dependency license texts are retained in `fixture-firmware/rust/licenses`;
  these notices are supplemental, not a claim of complete binary-license coverage.
- Rust dependencies retain their original licenses. Exact resolved names, versions,
  registry checksums and license expressions are in docs/RUST-DEPENDENCIES.md
  and the preserved native/HEX locks. The new target/GUI runtime graph is fixed
  by `fixture-firmware/runtime/Cargo.lock` and checked by
  `scripts/test-runtime-lock.py`; nested standalone locks do not govern it.
  Registry libraries are fetched, not vendored here.
- ESP-IDF (Apache-2.0), LVGL (MIT), Waveshare BSP and transitive managed components
  retain their distributed licenses and notices. Full component resolution/hashes
  are in fixture-firmware/app/dependencies.lock. Registry sources are not bundled.

- COIN adapter original MIT notice is retained verbatim in `fixture-firmware/coin/LICENSE`.
  Its 22 upstream-derived raw-bit golden fixtures and expanded fixture source retain
  the custom Ooga Booga license in that directory; see docs/COIN-MILESTONE.md
  and docs/coin-import-manifest.json. Existing HEX dependency and OFL font
  notices apply unchanged; adapter path dependency introduces no runtime crates.

Before binary distribution, collect and review all applicable dependency license
texts (including libsecp256k1, FreeType, libpng and zlib) from the exact resolved
sources. This source preparation is not an exhaustive legal compliance opinion.
No vendor endorsement, trademark license or upstream affiliation is claimed.

## DICE candidate

`fixture-firmware/dice` retains complete MIT and custom Ooga Booga licenses.
Its exact public upstream dice fixtures derive from the same pinned EntropyLab
revision and retain fixture source metadata. See `docs/dice-import-manifest.json`
and `docs/DICE-MILESTONE.md`; no upstream endorsement is implied.
The intentional Dice lock portability exception restores the exact repository
HEX dependency versions/checksums, including both hex-conservative versions.
`docs/RUST-DEPENDENCIES.md` already covers those versions and license expressions;
no registry package or license delta is introduced relative to the HEX runtime.
This does not replace the binary-distribution license review required above.

## Words runtime, dictionary and Orbit

- `fixture-firmware/runtime-sources/{hex-core,coin-core,dice-core,mnemonic-core}`
  are copied source cores compiled by `fixture-firmware/runtime`. Their complete
  local license files are retained: MIT for original native adapters and the
  exact custom Ooga Booga text for upstream-derived material where applicable.
  The HEX core's custom-license copy is attribution, not blanket dual licensing.
  Existing vector/dependency notices also apply to copied tests and fixtures.
- `fixture-firmware/app/main/bip39_dictionary.inc` identifies the English
  dictionary from `@scure/bip39` 2.4.0. Its complete MIT notice is retained at
  `fixture-firmware/LICENSE-dictionary.txt`; it is not covered merely by the
  native project's MIT notice.
- `fixture-firmware/app/main/assets` contains the authentic SVG, derived PNG and
  packed RGB565/A8 logo. All are artwork, including the C pixel representation.
  The exact `LICENSE-OOGA-BOOGA` is retained there. See the adjacent
  [asset provenance](fixture-firmware/app/main/assets/PROVENANCE.md) and portable
  hash manifest for origin and conversion scope.
- Logo inclusion follows the repository-wide software-license coverage
  interpretation supported by the pinned upstream README lines 570–575 and
  integrated tracked logo. No separate restriction was found in the bounded
  review; this is not an explicit separate artwork/trademark grant, a verified
  individual authorship chain, or a legal guarantee.

Credit **EntropyLab — Team Ooga Booga** and
[OogaBoogaX/entropylab](https://github.com/OogaBoogaX/entropylab). Mr.Hodl's public
[origin account](https://x.com/mrHodl/status/2099170677245014304) and
[calculator scope](https://x.com/mrHodl/status/2099506569931010421) inform attribution,
not a claim of sole authorship or logo ownership. This adaptation is unofficial;
no endorsement, affiliation, security certification or separate trademark license
is claimed. The exact custom license is not replaced by MIT or the Unlicense.
