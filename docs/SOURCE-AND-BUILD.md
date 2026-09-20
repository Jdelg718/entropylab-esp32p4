# Exact candidate02 source and build navigation

Publication HEAD is packaging, not the revision built into the firmware.
Firmware source is `2b919dc73c9cbcbb5e845850650d71c6782ad68b`;
the hardened recipe is `0ec7f6dd8ec62d6d554fea6726deeeb5a795a102`.
The extracted preview `source/build-recipe/` is archival, not a standalone kit.
External dependencies are pinned but not completely vendored; independent
reproducibility is NOT established. No rebuild accompanies publication.

```sh
git clone https://github.com/Jdelg718/entropylab-esp32p4.git
cd entropylab-esp32p4
git worktree add --detach ../candidate02-recipe 0ec7f6dd8ec62d6d554fea6726deeeb5a795a102
git worktree add --detach ../candidate02-source 2b919dc73c9cbcbb5e845850650d71c6782ad68b
cd ../candidate02-source
python3 ../candidate02-recipe/scripts/education-successor.py stage --destination "$HOME/education-candidate-rebuild/source"
```

The historical staging helper validates its own checkout before exporting Git objects.
Because recipe-only documentation may differ from the source snapshot, that check can
reject the recipe checkout. Resolve this with the original isolated build preparation
procedure before rebuilding; the commands above are navigation, not a verified fresh
build recipe. Publication does not claim that this route has been exercised.
Do not run its historical whole-checkout identity check on publication main.
Read [runner prerequisites](../scripts/run-education-build.py) and
[build recipe](../scripts/build-education-successor.sh). Prepare isolated writable
HOME, TMPDIR, CARGO_HOME and IDF_COMPONENT_CACHE_PATH, pinned managed components
and Cargo downloads, and the external input inventory required by the runner.
Set IDF_PATH, IDF_TOOLS_PATH, RUSTUP_HOME and Rust proxy PATH explicitly.
IDF must be clean recursive revision `b774170ff46c393eeb5e495ea37936038d3f4f4f`;
Rust is nightly-2026-04-15 with rust-src and target riscv32imafc-esp-espidf.
Retain component/Cargo locks and Rev1.3 defaults. No old build objects are permitted.
The external inventory format and validation are defined in the pinned runner;
its checksum must be independently recorded, not copied from an untrusted input.

```sh
# EXTERNAL_INPUTS and EXTERNAL_SHA256 identify your prepared inventory.
python3 scripts/run-education-build.py "$HOME/education-candidate-rebuild" \
  --external-inputs "$EXTERNAL_INPUTS" --external-inputs-sha256 "$EXTERNAL_SHA256"
python3 scripts/verify-education-build.py "$HOME/education-candidate-rebuild"
python3 scripts/verify-education-runtime.py "$HOME/education-candidate-rebuild" --recipe
```

These commands describe the existing gated route, not a promise that a fresh
machine already has its dependencies. A failed receipt remains failed; do not
use --inspect-failed as an acceptance bypass. The complete helpers are in the
pinned repository worktree, not the partial archived build-recipe directory.

[Current disposition and exact images](PUBLICATION-CANDIDATE02.md).
Packaged source notice links are relative to the packaged source root; their
original texts and historical review findings have not been rewritten.
