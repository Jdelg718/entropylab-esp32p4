# Candidate02 publication status

This is an additive, locally prepared main successor, not an already published
release. Independent frozen-candidate review and binary notice closure are required
before push/merge or binary distribution. No prerelease assets are approved.

## Exact tested package and provenance

The entire `preview/education-candidate02-dev-test-01/` subtree is the unchanged
690-member extraction of `education-candidate02-dev-test-01.zip`, SHA-256
`6ff7ca8779850f1277e61a5d7dc875e24fd1c4c921601a4ea47b184e6d497cd9`.
`verify-publication.py` enforces its manifest, source and binary bindings. The server
routes `/` to this namespace. `/flash/first-install/` remains historical and serves
the previous firmware; it is not the current onboarding URL.

| Identity | Value |
|---|---|
| Firmware source revision | `2b919dc73c9cbcbb5e845850650d71c6782ad68b` |
| Build recipe revision | `0ec7f6dd8ec62d6d554fea6726deeeb5a795a102` |
| Source manifest SHA-256 | `66231dae53a45e30470f7f26ab50cb8a9aff3d500aafb7c78fcdedfd755bbb13` |
| App SHA-256 | `42d5e3b40a3869157171552162189a281c84b893c8533fe3ae8266bfc4c47550` |
| Bootloader SHA-256 | `4f00f81aad82838f4e33555e322abfbfff7d1de947d339c86e50be820cbd7bb4` |
| Partition table SHA-256 | `d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb` |

Independent image/source and supplemental runtime review passed for these bytes.
No build is rerun for this documentation/server/package integration. The package's
HOLD and NOT TESTED fields remain immutable historical statements, not a denial
of the subsequent user-operated one-board acceptance. The owner reported
“everything works” after an installation screenshot showed image and erased-tail
verification; this is not independent reproducibility or full recovery testing.

## License gate — still HOLD, not silently cleared

The corresponding project sources, source manifest, lockfiles, exact original
licenses and 282 collected notice files are retained in the immutable package.
The existing notice review explicitly identifies unresolved issues in
[the notice checklist](release-notices/CHECKLIST.md):

- `hex_lit 0.1.1` declares MITNFA but its exact package does not contain a separate
  copyright notice. The supplied SPDX text does not resolve attribution sufficiency.
- Exact IDF embedded per-file exceptions and vendor binary/library terms still
  need a recorded determination. Newlib/libgcc/GCC Runtime Library Exception and
  Rust core/alloc notices are collected, not automatically approved.
- The dependency notice collection originated in the predecessor build. Exact
  candidate02 link inputs must be reconciled to it before claiming full coverage.

FreeType uses the FTL option, not GPL. Portions of this software are copyright ©
The FreeType Project (www.freetype.org). All rights reserved. The custom Ooga Booga,
BSD, MIT, OFL, Apache and other original texts are preserved; no blanket relicensing
or endorsement is claimed. This bounded inspection is not legal advice.

**Do not upload a prerelease ZIP or merge these binary additions until these
concrete notice items and independent review are resolved.** User authorization
permits publication but cannot itself satisfy third-party license obligations.

## Testing boundaries

No BitBox hardware was available. BitBox-style dice compatibility is unproven;
please compare public known vectors, never secrets. Host/Node/browser tests are
not hardware simulation equivalence. Native macOS/Windows, independent clean
rebuild reproducibility, recovery and other board qualification remain untested.
The historical broad `test-host.sh` pins predecessor bytes and can reject this
successor; do not rewrite historical pins to manufacture a pass. Use the successor
checks and record the historical failure separately.
