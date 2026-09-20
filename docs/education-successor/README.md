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

## Completed local attempt and supplemental checks

The first local attempt completed compilation, linking and image generation but
**the runner exited 1**. The immutable staged `verify-runtime-elf.py` expects
`fixture_alloc` to call `heap_caps_aligned_alloc`. Its expectation predates the
private 8 KiB `multi_heap` arena in `fixture_diagnostics.c`, introduced by source
integration commit `d7258eb37c70ffddb1970fdfec9fe65ea5682d9d` without updating that
verifier. This is a stale gate, not evidence of a compiler miscompile:
`fixture_alloc` calls `multi_heap_aligned_alloc` in the actual ELF, matching the
staged source. `fixture_free` tail-calls `multi_heap_aligned_free`, which `nm`
proves is the same address as `multi_heap_free`. Symbol alias spelling alone must
not approve a different callee address.

The failed receipt, stream, source and original verifier remain unchanged.
The new **supplemental** runtime checker retains the original ABI, API, panic,
map and duplicate-definition checks, requires private allocator/free callees
(address-checked against the ELF symbol table), rejects shared-heap shim calls,
and checks arena size and initialization. This does not retroactively change the
runner result or amend an accepted-source identity.

Run against an existing completed candidate, without recompiling:

```sh
python3 scripts/verify-education-build.py /owned/path/education-candidate-N
# A completed FAILED receipt is rejected by default. To inspect its outputs:
python3 scripts/verify-education-build.py /owned/path/education-candidate-N --inspect-failed
# The inspection still exits 2 and reports INSPECTED-build-failed, never PASS-build.
python3 scripts/verify-education-runtime.py /owned/path/education-candidate-N --tool-prefix /pinned/toolchain/bin/riscv32-esp-elf-
python3 scripts/test-education-build.py /owned/path/education-candidate-N
python3 scripts/test-education-runtime.py
```

The image checker recomputes source, managed-source, recipe, stream, exports,
ELF and artifact bindings; validates image checksum/digest, descriptor/ELF hash,
chip/revision, generated configuration, partition digest/fit and flash offsets;
and scans app/boot images for known local-path prefixes and new lesson strings.
Generated configuration hashes are observations at inspection time, not additions
to the original build receipt. Local receipts are not signatures or independent
provenance. The path-prefix scan is not a general privacy proof. The runtime
checker alone is not a source-manifest verifier; use it alongside the image check.

Regression tests cover image corruption/truncation/trailing bytes, configuration
mutations, incomplete artifact/recipe inventories, failed-receipt status handling,
private/shared heap calls, missing calls, and matching/mismatching alias addresses.
The current image regression integration test deliberately uses the failed first
candidate and asserts that its failed status is preserved.

Local evidence: application SHA-256
`6fefe70fde3bf9eccd2fdd898a5ec438bf3020fe79e08b7d9b2ff3a3bbdf6fb4`,
ELF SHA-256 `da7ea637efab245efa19071f0e88140fb2c04aa6445f516e8ab746cfd610825d`,
descriptor `e66231dae53a45e30470f7f26-e1`.
The application occupies 1,542,288 bytes of its 8 MiB factory partition.
Supplemental image/configuration and runtime checks passed; the original runner
and original host gate still failed. Hardware, rendering/touch on device,
reproducibility, release acceptance and distribution remain **HOLD / NOT TESTED**.
