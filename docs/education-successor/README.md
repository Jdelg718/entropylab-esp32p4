# Education successor: candidate-only source/build boundary

This additive identity represents source revision
`2b919dc73c9cbcbb5e845850650d71c6782ad68b`, not the accepted firmware.
`source.json` binds every tracked file at that revision to SHA-256 and size.
The accepted archive is unchanged. Comparing its exact firmware inventory with
this revision permits exactly one difference: `education_content.inc`.
No historical pins, archives, binaries or acceptance records are rewritten.

## Gates

- `python3 scripts/education-successor.py check`: recomputes identity from immutable
  Git objects, checks the accepted archive and exact firmware delta, then checks
  current source bytes. Git objects for the named revision are required.
- `python3 scripts/test-education-successor.py`: valid serialization control and
  semantic mutations (including revised hashes), current bytes, extra input and
  symlink rejection. This does not certify target execution.
- `python3 scripts/education-successor.py stage --destination /owned/path/education-candidate-N/source`:
  creates a new, exact source-only tree; refuses an existing directory. This is
  not an accepted-source archive and must not be packaged as one.
- Copy pinned managed components and Cargo downloads into private candidate
  directories. No prior objects, runtime archives or build trees may be reused.
  Configure `IDF_PATH`, `IDF_TOOLS_PATH`, Rust proxy `PATH`, `RUSTUP_HOME`, private
  `CARGO_HOME` and `IDF_COMPONENT_CACHE_PATH`, `HOME` and `TMPDIR` before running
  `python3 scripts/run-education-build.py /owned/path/education-candidate-N`.
  Read-only pinned toolchain installations may be shared. Commit the recipe first.

The bounded runner records the real child exit code, exact command/environment,
source manifest/recipe revisions and hashes, remap exports, managed input
inventory before/after, stream hash and generated output hashes. It rejects dirty
recipes, pre-existing build trees and changed source/configuration. The recipe
uses IDF `b774170ff46c393eeb5e495ea37936038d3f4f4f`, nightly-2026-04-15,
`riscv32imafc-esp-espidf`, original component/Cargo locks, existing baseline and
Rev1.3 defaults. The `e<manifest-prefix>-e1` descriptor is only an index into the
full external manifest. Existing generic remap, image/ABI/runtime verifiers are
reused; historical accepted-source exporters are not invoked or bypassed.

A successful local build means compilation plus those explicit checks only.
Independent image/partition/descriptor review, runnable-binary privacy scans,
resource review, lesson rendering and touch behavior on hardware, reproducibility,
release packaging and publication remain separate gates. Distribution is HOLD.
The original `scripts/test-host.sh` is still run as directed by AGENTS but currently
fails on a historical `gui.c` pin; that failure is not suppressed or called PASS.
The accepted-source runner must continue rejecting the new education source.
Use the live education host tests for the changed lessons without transferring
old acceptance to the candidate.
