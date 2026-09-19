> **Historical source-bound record, not current release/feature status.** Evidence and old “current” labels below apply only to the named predecessor identity. For published retention-preview assets, accepted tuple, implemented features and remaining holds see [current status](../../CURRENT-STATUS.md) and [first-install guide](../../PUBLIC-FIRST-INSTALL.md). Current host entrypoint: `python3 scripts/retention-host.py`; predecessor runners intentionally retain their original pins.

# DICE local publication candidate — not release approval

Offline seed/mnemonic + fingerprint generator, not a signer. Public laboratory
inputs only; never fund these outputs. No RNG, signing, networking, persistence,
SD writes or arbitrary-code features added. microSD remains design-gated.

## Contract

Strict native ASCII 1–6, 1..1024 bytes, selectors 12/15/18/21/24. Raw
COLDCARD-style hashes the complete ASCII transcript with SHA256; Coleman maps
6 to 0 before hashing. Selected leading 128/160/192/224/256 bits feed the existing
HEX BIP39/BIP32/BIP84 path. Modes retain separate transcripts. Nominal thresholds
50/62/75/87/100 assume fair independent rolls, not measured source quality.
Adapter return 1 is successful **WEAK_INPUT_LAB_ONLY**, negative values are
errors, 0 is count-only success, never an assurance badge. Warnings persist
through computation, result and failures. Editing invalidates old results.

## Exactly two user-reported hardware observations

On the frozen source image, with visible weak warnings:
- D6 raw, 12 words, public input `123456`, fingerprint `8ab125dd`.
- D6 6->0, 12 words, public input `123456`, fingerprint `d6dd3456`.

These short predictable public inputs are not threshold testing, soak testing,
readiness approval, physical entropy proof or a runtime claim for a repository
rebuild. No hardware was flashed or accessed during this import.

## Source identity and exceptions

Frozen source aggregate SHA256:
`859dbd362dd77684ffdb570c8b37148d107bf1574a09b0dcf950761ed809b8e2`.
Frozen application SHA256:
`ceb685d6d275712f502f975a8fc012c6e273f4005601581fb60a68cfa5c4cab9`.
The import manifest maps relative source identities and exact byte exceptions.
Only the app worker/GUI/header delta, adapter, public fixtures/tests and full
licenses are imported. Existing main worker, HEX core, Coin core, pins and
component resolution are retained. The source GUI header's LVGL dependency is
not reintroduced: GUI C files include LVGL; worker headers remain LVGL-free.
Portable Cargo paths refer to the repository HEX core. No external prelinked
archives are inputs: target builds compile one dice archive containing the HEX
runtime and the existing localized coin archive, with no second HEX archive.

## Effective dependency portability exception

The imported source Dice lock originally upgraded inherited HEX transitives.
The candidate intentionally differs from that reviewed source lock: precise
offline Cargo updates restore cc 1.4.4, find-msvc-tools 0.1.11,
hex-conservative 0.2.2 and 1.2.0, and syn 3.0.4, including their original
registry checksums. All inherited package identities and dependency edges now
match the preserved repository HEX lock, not merely direct Cargo.toml pins.
`scripts/test-dice-lock.py` checks the complete multiversion graph; it failed on
the imported lock before the fix. HEX/Coin code and historical locks are untouched.
This is the seventh documented import exception. Dice source code and frozen
source/archive identities are unchanged, but this lock and rebuilt binaries are
not byte-identical to the reviewed source. New repository binary tests are
mandatory; the two source hardware observations do not validate this rebuild.
License metadata already covers these exact restored versions in
`RUST-DEPENDENCIES.md`; no new registry package or license expression is added.

## Verification commands and boundaries

Run `bash scripts/test-host.sh`, `bash scripts/test-gui-host.sh`, then
`bash scripts/build-firmware.sh` with the existing documented pinned toolchains.
Use `CARGO`, `CARGO_HOME`, `RUSTUP_HOME`, PATH, `HOST_TOOLCHAIN`, `IDF_PATH`,
`IDF_TOOLS_PATH`, `LVGL_SOURCE_DIR` and `BUILD_JOBS` for portable local setup.
Offline cached Rust dependencies may be enforced with `CARGO_NET_OFFLINE=true`.
Existing IDF/component/LVGL caches may be reused; no clean-download reproducibility
claim is made. Generated build products and logs remain ignored.
Host tests do not imply target or hardware PASS. Target gates check Rev100–199,
32MB flash, 200MHz PSRAM, single-float ABI, unique APIs, aborting panic and
allocator/free hooks. Publication still requires independent SPEC/security,
privacy and license review plus explicit approval. No commit/push/merge/flash.

## Actual local candidate results after effective-lock repair

- `scripts/test-host.sh`: PASS; 68 Rust test executions across debug/release,
  zero failed; drift harness records one pre-existing skipped test. Dice worker:
  20 original vectors, 14 accepted/6 rejected, full1024, both mappings, all
  threshold boundaries including 99/100, persistent weak metadata and failures.
  Composed Coin worker: 22 upstream vectors PASS. HEX regressions PASS.
- `scripts/test-gui-host.sh`: PASS, actual LVGL/C/Rust composition; HEX 1822
  geometry checks, Coin and Dice mode isolation, all selectors, full1024 owned
  snapshots, busy controls, weak result/failure warnings and invalidation.
- `scripts/build-firmware.sh`: PASS from repository sources with existing caches;
  Rev100–199, 32MB flash, PSRAM200, ilp32f. Dice archive 84 members, localized Coin
  33 members; sole runtime, unique APIs/panic and allocation/free hooks verified.
  Current repository main/GUI/header object freshness verified; component and
  existing HEX/Coin locks unchanged. This is not a fresh-download rebuild.
- Application SHA256:
  `ade5e437fc3037574467b240331747ee69914a62b0887eb8ede948031a0caeb2`.
  ELF SHA256:
  `448ce364bb5cbded237924a043a1860edf41922f7bf3fff846c1f306e3b44bc3`.
  These repository artifacts differ from the source image and are not hardware tested.

Ignored repair evidence: `fixture-firmware/logs/dice-lock-red.log`,
`dice-lock-host.log`, `dice-lock-gui.log`, `dice-lock-target.log`,
`verification.json`, and GUI PPMs under `logs/gui-host/`.
The effective-lock regression first failed on the imported drift, then passed
with both multiversion/identity tests. Host and GUI reruns and the actual target
rebuild all exited 0 using offline locked Cargo and existing IDF 5.5.5 caches.
An initial target setup attempt stopped before compilation: Python 3.13 selected
a nonexistent IDF venv. Scoped bootstrap Python 3.12 plus unsetting inherited
Python/virtualenv variables restored the existing IDF environment; no install
or dependency change was needed. Target retry ran every existing verifier.

## SECURITY portability repair and renewed review freeze

The earlier target PASS reused a historical HEX archive only in the Coin
localizer's panic/composition checks. That hidden input is now removed:
`localize-coin.py` requires `--runtime-archive`, and `build-firmware.sh` passes
the freshly built Dice archive. Both the global panic ownership check and strict
relocatable composition use that explicit archive. Exact global-symbol delta,
local panic, ordered archive-member allocated payload identity and strict linker
behavior are unchanged; no duplicate-definition suppression was added.

TDD: `python3 scripts/test-coin-archive.py` first failed with the old localizer
because its historical HEX target was absent in an isolated fixture tree. The
same command now passes all eight unchanged/payload/addition/removal/missing/
extra/reordered/malformed cases using only a Dice runtime fixture, plus explicit
missing-runtime and omitted-argument fail-closed checks. Evidence:
`fixture-firmware/logs/dice-portability-red.log` and `dice-portability-green.log`.

Actual full rerun: host, GUI and target all passed using scoped Python 3.12.13,
Rust host 1.95.0, target nightly-2026-04-15, pinned IDF, locked Cargo with
`CARGO_NET_OFFLINE=true`, and existing caches. The target proof temporarily moved
only the historical target HEX archive, ran the full build while it was absent,
asserted it remained absent, and restored it in `finally` with an identical SHA256
(`18c43247c9cc294e5a17490bd65741854c048105a3fe45e68bfac1d072dd1ba3`).
It performed an actual final ELF relink; this is not a grep-only proof. All 26
inherited dependency records match, including checksums and dependency edges.
The existing final verifier passed unique Dice/Coin/HEX APIs, sole Dice-archive
runtime, aborting global panic, allocation/free hooks and no duplicate-ignore flags.
The application, ELF, Dice archive and lock hashes reported here remain unchanged
and were remeasured after this repair; localized Coin archive SHA256 is
`dd223d6256c1fca5165be9f790315bfd4e2ef63830515e45a9dcfc3a4de696bb`.

Ignored reproducible local commands and evidence are `logs/dice-portability-run.sh`,
`logs/dice-portability-target.py`, `logs/dice-portability-{host,gui,target,run}.log`
and `logs/dice-portability-artifacts.json` under `fixture-firmware/`.
Setup retries are retained separately: the first Rust home caused rustup to fetch
nightly components; the final run uses the pre-existing integration toolchain
with auto-install disabled. A mixed host toolchain link failed and was rerun with
host 1.95.0 consistently. A target retry lacked Ninja on PATH; adding the existing
bootstrap bin resolved it, and the failed attempt also restored the historical
archive. No dependency locks or application behavior changed for this repair.

Freeze scope for renewed SPEC then SECURITY review: only the localizer, its
archive regression test, build invocation and this evidence addendum changed.
`logs/dice-portability-review-sha256.json` records candidate source hashes.
This is not independent approval or hardware evidence. No flash, commit or push;
all historical sources and device backups remain untouched.
Dice lock SHA256:
`870afcf761cc2ae30339189e610afc3bd1630e59978e23cf1b4bb874a4f03762`.
Target Dice archive SHA256:
`7a46721bba260163c4e6cfac58298239294da4a36ad4150fc98b96436b6bed1b`.
Initial host attempts exposed a missing rustc PATH and an inherited source-relative
contract-test path; both were corrected, preserving offline and locked builds.
