# Third-party notices

This is a mixed-license source distribution; not a blanket MIT relicensing.

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
- Rust dependencies retain their original licenses. Exact resolved names, versions,
  registry checksums and license expressions are in docs/RUST-DEPENDENCIES.md
  and both Cargo.lock files. Libraries are fetched, not vendored here.
- ESP-IDF (Apache-2.0), LVGL (MIT), Waveshare BSP and transitive managed components
  retain their distributed licenses and notices. Full component resolution/hashes
  are in fixture-firmware/app/dependencies.lock. Registry sources are not bundled.

Before binary distribution, collect and review all applicable dependency license
texts (including libsecp256k1, FreeType, libpng and zlib) from the exact resolved
sources. This source preparation is not an exhaustive legal compliance opinion.
No vendor endorsement, trademark license or upstream affiliation is claimed.
