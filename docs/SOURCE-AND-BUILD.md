# Exact candidate02 source and build navigation

Publication HEAD is packaging, not the revision built into the firmware.
Firmware source is `2b919dc73c9cbcbb5e845850650d71c6782ad68b`;
the hardened recipe is `0ec7f6dd8ec62d6d554fea6726deeeb5a795a102`.
The extracted preview `source/build-recipe/` is archival, not a standalone kit.
External dependencies are pinned but not completely vendored; independent
reproducibility is NOT established. No rebuild accompanies publication.

## Verified source retrieval and staging (no compiler required)

Run in a new workspace with Git, Python 3 and an owned, non-group/world-writable
HOME without symlink ancestors. The candidate directory must not already exist.
The public clone must contain both pinned commits; stop if either is unavailable.

```sh
set -eu
git clone https://github.com/Jdelg718/entropylab-esp32p4.git
cd entropylab-esp32p4
git worktree add --detach ../candidate02-recipe 0ec7f6dd8ec62d6d554fea6726deeeb5a795a102
git worktree add --detach ../candidate02-source 2b919dc73c9cbcbb5e845850650d71c6782ad68b
RECIPE=$(cd ../candidate02-recipe && pwd -P)
SOURCE=$(cd ../candidate02-source && pwd -P)
CANDIDATE="$HOME/education-candidate-rebuild"
test "$(git -C "$RECIPE" rev-parse HEAD)" = 0ec7f6dd8ec62d6d554fea6726deeeb5a795a102
test "$(git -C "$SOURCE" rev-parse HEAD)" = 2b919dc73c9cbcbb5e845850650d71c6782ad68b
test -z "$(git -C "$RECIPE" status --porcelain)"
test -z "$(git -C "$SOURCE" status --porcelain)"
python3 -B "$RECIPE/scripts/education-successor.py" check
python3 -B "$RECIPE/scripts/education-successor.py" stage --destination "$CANDIDATE/source"
```

The helper anchors its checkout to its own file, **not the working directory**.
The exact pinned recipe passes: its changes relative to the pinned source are
additions, not modifications to the source manifest's entries. Staging exports
immutable source Git objects and checks every staged file's size and SHA-256,
including exact inventory equality. The source worktree is a separate inspection
checkout; it is not modified or used to supply recipe files. Do not copy recipe
files into it or run the publication-main helper instead: publication-main
correctly fails historical source identity even when invoked from the source
worktree. No identity exceptions or manifest changes are needed.

This source staging route was exercised using local Git objects at both exact
revisions; a fresh public network clone and a full firmware compile were not
performed for this documentation correction.

## Local inventory provisioning and gated build (not a portable bootstrap)

Keep the shell variables above. Read the complete pinned helpers in `$RECIPE`,
particularly `scripts/run-education-build.py` and
`scripts/build-education-successor.sh`. The runner creates isolated writable
HOME, TMPDIR, CARGO_HOME and IDF_COMPONENT_CACHE_PATH. Supply pinned managed
components, Cargo downloads and the external input inventory required by the
runner. That inventory binds local absolute installation paths, physical entries
and resolved links; it is not a portable dependency lock or an automatic installer.
Prepare its IDF_PATH, IDF_TOOLS_PATH, RUSTUP_HOME and Rust proxy TOOL_BIN roots
explicitly. IDF must be clean recursive revision
`b774170ff46c393eeb5e495ea37936038d3f4f4f`; Rust is nightly-2026-04-15 with
rust-src and target riscv32imafc-esp-espidf.
Retain component/Cargo locks and Rev1.3 defaults. No old build objects are permitted.
The external inventory format and validation are defined in the pinned runner;
its checksum must be independently recorded, not copied from an untrusted input.

Only after local inventory provisioning (not verified by the staging test):

```sh
# EXTERNAL_INPUTS and EXTERNAL_SHA256 identify your independently approved inventory.
python3 -B "$RECIPE/scripts/run-education-build.py" "$CANDIDATE" \
  --external-inputs "$EXTERNAL_INPUTS" --external-inputs-sha256 "$EXTERNAL_SHA256"
# Completed v2 receipts require the separate, pinned supplemental verifier.
git worktree add --detach ../candidate02-verifier 33c90ec8d1b957e363690fa7912e250e4782fa11
VERIFIER=$(cd ../candidate02-verifier && pwd -P)
test "$(git -C "$VERIFIER" rev-parse HEAD)" = 33c90ec8d1b957e363690fa7912e250e4782fa11
test -z "$(git -C "$VERIFIER" status --porcelain)"
# Supply TOOL_PREFIX from your actual pinned RISC-V installation, ending in
# /bin/riscv32-esp-elf- (the executable prefix, not just its directory).
: "${TOOL_PREFIX:?Supply the actual pinned RISC-V executable prefix}"
python3 -B "$VERIFIER/scripts/verify-education-build.py" "$CANDIDATE" \
  --external-inputs "$EXTERNAL_INPUTS"
python3 -B "$VERIFIER/scripts/verify-education-runtime.py" "$CANDIDATE" \
  --tool-prefix "$TOOL_PREFIX"
# Do not use --recipe for a completed build; it is a pre-build-only check.
```

These commands describe the existing gated route, not a promise that a fresh
machine already has its dependencies. A failed receipt remains failed; do not
use --inspect-failed as an acceptance bypass. The complete helpers are in the
pinned repository worktree, not the partial archived build-recipe directory.

[Current disposition and exact images](PUBLICATION-CANDIDATE02.md).
Packaged source notice links are relative to the packaged source root; their
original texts and historical review findings have not been rewritten.
