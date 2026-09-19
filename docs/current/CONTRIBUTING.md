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

## Check out and test

```sh
git clone https://github.com/Jdelg718/entropylab-esp32p4.git
cd entropylab-esp32p4
git checkout main
git rev-parse HEAD
python3 scripts/retention-host.py --check
python3 scripts/retention-host.py
```

Record the SHA in reports. To inspect the released snapshot instead, use `git checkout --detach v0.1.0-retention-preview`. Other public branches are feature/historical snapshots, not install approval. Follow the [retention host prerequisites](../retention-host/README.md); keep all source pins and lockfiles unchanged. Predecessor runners intentionally reject successor identities. [Original education/docs147 failures](../HISTORICAL-CONTRIBUTING.md) are preserved, not current failures or newly rerun results. A target build requires separate scope and never authorizes hardware access.

## Keep changes reviewable

1. Start from the current main or named release tag and state its commit SHA.
2. Keep one concern per pull request.
3. Explain behavior, safety impact, source/provenance impact, and test coverage.
4. Preserve production semantics unless the change explicitly proposes and tests new behavior.
5. Preserve third-party notices and source-local licenses. Document new sources and exact revisions.
6. Run the relevant focused tests plus the current `python3 scripts/retention-host.py` gate when feasible.
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
