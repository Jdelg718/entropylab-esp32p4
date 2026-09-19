> **Current guidance:** [CONTRIBUTING — current retention preview](docs/current/CONTRIBUTING.md).
> The text below is the historical source checkpoint, not current release status.
> v0.1.0-retention-preview is published; v0.1.1 is a local documentation/onboarding patch pending publication.
> Firmware is unchanged; public-policy hardware qualification and the separate retention updater remain held.

# Contributing to EntropyLab

Thank you for helping improve EntropyLab. We welcome small, reviewable changes to public-fixture tests, educational text, UI accessibility, portability, hardware support research, and documentation.

## Safety boundary

This repository is an experimental educational project, not a wallet or signer. Use public practice fixtures only.

Do not submit:

- real seeds, mnemonics, passphrases, private keys, addresses tied to funds, or wallet exports;
- flash dumps, device identifiers, MAC addresses, serial numbers, private paths, IP addresses, credentials, tokens, or unsanitized logs;
- generated firmware binaries, build directories, caches, or toolchain installations;
- claims that a mockup, host test, successful compile, or checksum proves physical-device behavior or entropy quality.

Do not add signing, transactions, persistence, networking, random-key generation, erase/eFuse operations, secure-boot changes, or flash-encryption changes as incidental work. Those require separate design and security review.

## Check out the publication branch

The proposed dedicated branch is `release/education-ui-20260917-docs147`. It is not on the public remote until the exact package passes independent publication review and is pushed. After publication:

```sh
git clone https://github.com/Jdelg718/entropylab-esp32p4.git
cd entropylab-esp32p4
git fetch origin release/education-ui-20260917-docs147
git checkout --detach origin/release/education-ui-20260917-docs147
git rev-parse HEAD
```

Record the printed commit SHA in every bug report or test result. A branch push is not a merge to `main` and does not create a tag or downloadable firmware release.

## Set up and test

Follow [docs/BUILD-FLASH.md](docs/BUILD-FLASH.md). The intended full host command is:

```sh
rustup toolchain install 1.95.0 --profile minimal
export LVGL_SOURCE_DIR="$HOME/entropylab-deps/lvgl"
bash scripts/test-host.sh
```

Use LVGL commit `85aa60d18b3d5e5588d7b247abf90198f07c8a63`. All Cargo commands use lockfiles. Do not refresh a lockfile merely to make a failing test pass.

Current publication gap: a clean package run reaches many passing Rust/vector checks, then `scripts/test-host.sh` exits 1 because `verify-dice-import.py` still pins a historical `gui.c` identity. A separate clean `scripts/test-gui-host.sh` build completes and then exits 134 on a stale Safety/About caption assertion. Report these exact known failures; do not call the host gate green and do not rebaseline them incidentally. The accepted modal140 focused host binary separately passed its eleven scoped executions.

Target changes must also pass the pinned target build with ESP-IDF v5.5.5 commit `b774170ff46c393eeb5e495ea37936038d3f4f4f` and Rust `nightly-2026-04-15`. A target build is not permission to connect or flash hardware.

## Keep changes reviewable

1. Start from the exact publication branch and state its commit SHA.
2. Keep one concern per pull request.
3. Explain behavior, safety impact, source/provenance impact, and test coverage.
4. Preserve production semantics unless the change explicitly proposes and tests new behavior.
5. Preserve third-party notices and source-local licenses. Document new sources and exact revisions.
6. Run the relevant focused tests plus `bash scripts/test-host.sh` when feasible.
7. Do not replace failing vectors or expected outputs without a separately justified baseline review.
8. Do not commit generated `build/`, `target/`, `managed_components/`, logs, caches, or binaries.

## Reproducing a bug

Include:

- repository commit SHA and branch name;
- board product name and printed/observed ESP32-P4 silicon revision;
- display size/resolution and whether the board is standard SKU 33874 or camera SKU 33875;
- ESP-IDF, Rust, compiler, CMake, Ninja, Python, and host OS versions;
- exact command and exit code;
- selected input family/method, word count, and a clearly labeled public fixture;
- expected and actual behavior;
- the smallest sanitized log excerpt that demonstrates the failure;
- sanitized screenshots with no real secrets, device IDs, private paths, or unrelated desktop content;
- whether the result came from host tests, a build, or physical hardware.

A useful UI report also gives the touch sequence and whether the contact was a tap, hold, drag, overlap, or wake action. Never reproduce with a funded or private phrase.

## Pull-request review

A maintainer review is not automatically a security or publication approval. Changes touching input semantics, cryptography, secret lifetime, dependencies, licenses, flashing, board configuration, or hardware support need the appropriate independent review. Physical-device claims require separately authorized testing on the named revision.

Future screen sizes and devices are roadmap work. A new layout or board adapter should begin as an explicitly unsupported experiment with public fixtures, then earn support through source review, host tests, target build, and physical acceptance. Do not broaden the compatibility table based on compilation alone.
