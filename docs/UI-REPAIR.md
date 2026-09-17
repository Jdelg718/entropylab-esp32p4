> Historical predecessor evidence only. Not current D6 build, installed identity or publication approval. See [current status](PUBLICATION-STATUS.md).

# Native UI repair: distinct source identity

This is a new uninstalled UI-repair candidate, **not exact installed combined
source** and not a reproducible-binary claim. Historical Dice, Words, native-edit,
combined-import and installed build records remain unchanged. Statements about
installed hardware or historical successful builds in older milestone documents
refer only to their recorded revisions.

## Provenance boundary

`ui-repair-successors.json` is a finite 15-entry record of the independently
reviewed code/test delta. SHA-256:
`5770532b54d8c6e9f3cc43515158c9a98b4e3d06254c722e82ffe0f550141513`.
The verifier pins this manifest, authenticates every final destination, preserves
the original Dice → Words → native edit → combined chain, and checks the exact
combined-to-repair before hash for each overlapping imported path. Other reviewed
UI paths carry preserved-package before hashes (null only for additions), checked
against the frozen package during packaging. The record does not authorize future
rebaselining. Packaging-only verifier/tests/docs are separately identified by the
final source inventory and diff evidence. Approved native production bytes were
not changed by packaging.

`test-dice-import.py` exercises unchanged historical entries, historical and
successor manifest mutation, every one of the 15 UI destinations, and malformed,
duplicate, removed, before/after-hash mutations of both combined and UI manifests.
No provenance gate is skipped or weakened.

## Omission reconciliation

`scripts/build-firmware.sh` restored byte-for-byte from the preserved package:
`8774802e3eddee4807a648b628cb9710b3b88cef764b2ed37cfe4f11fdffacb0`.
All five omitted LifeHash files are **historical generated test evidence**, not
required source: generated PPM/RGB images, their checksum list, a one-run Linux
resource report and a standalone sanitizer PASS line. They remain intentionally
excluded rather than being presented as current repair evidence. Exact original
hashes, sizes and dispositions are in `ui-repair-evidence-exclusions.json`.
The original package is unchanged; no binary evidence is reintroduced. Authentic
application artwork and license files remain preserved.

## Validation / handoff

The parent reports a fresh independent target build, all three command exits 0,
and zero app/main differences against the built snapshot. Recorded target hashes:

- Application bin: `3a83989d5ef221b0081230c5a7a0b030ed2c652db09e1e0817dee037ecd5432e`
- ELF: `ed77cecb3a67b4c26e50e9b98cea210906790cdda7768418bed9ee06d2a365e9`

These are new build identities, not installed firmware identities. Full host and
provenance execution status is recorded by the adjacent diagnosis runner with
command, true exit and log SHA-256; see packaging handoff for final results.
Host tests, target compilation, visual behavior on real hardware and secure
erasure are distinct assurance boundaries. No flash/device/commit/push occurred.
No real-secret use, physical freeze-resolution or publication approval is claimed.
