# EntropyLab — unofficial ESP32-P4

## v0.1.1-retention-preview — documentation/onboarding patch

**v0.1.1-retention-preview** is the published documentation/onboarding patch with unchanged firmware. The previous **v0.1.0-retention-preview** release and its assets remain immutable. Existing v0.1.0 users do not need to reflash.

Download the [published prerelease](https://github.com/Jdelg718/entropylab-esp32p4/releases/tag/v0.1.1-retention-preview). Read [the first-install guide](docs/PUBLIC-FIRST-INSTALL.md) before replacing firmware: exact board confirmation, backup/export and destructive consent are required.

From a clean clone or verified extracted ZIP, run `python3 scripts/serve-public.py` on Linux/macOS or `py -3 scripts/serve-public.py` on Windows, then open http://localhost:8000/flash/first-install/ in desktop Chrome or Edge. No public hosted installer is configured. Native Windows/macOS execution remains untested.

**Public practice only — not a wallet or signer.** Never enter real secrets or fund displayed addresses. This independently maintained native adaptation is not shipped, audited or endorsed by OogaBoogaX or Waveshare. Input is supplied by the user; encoding, hashing and visual fingerprints do not certify randomness.

The exact firmware tuple has prior one-board private-path acceptance; the new public install policy is hardware-unexercised. The separate retention updater remains held. Read [current release scope and remaining holds](docs/CURRENT-STATUS.md), the single current status entrypoint; readback does not establish boot or recovery.

## Start here

- [Installation, manual RESET and evidence limits](docs/PUBLIC-FIRST-INSTALL.md)
- [Contributing and current host tests](docs/current/CONTRIBUTING.md) · [portable retention host gate](docs/retention-host/README.md)
- [Hardware identifiers](docs/current/docs/HARDWARE.md): exact Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, PCB rev1.3, 32 MiB. Silicon 100–199 alone does not identify PCB revision.
- [Approved priority order](docs/current/ROADMAP.md) · [security boundary](docs/current/SECURITY.md)
- [Provenance context](docs/CURRENT-STATUS.md#provenance-context) · [third-party notices](THIRD_PARTY_NOTICES.md)

## What is implemented

The 480 × 800 LVGL interface groups ten input routes into seven families:

| Family or method | Implemented behavior | Important boundary |
| --- | --- | --- |
| Hex | Accepts exactly 32/40/48/56/64 hexadecimal characters for 12/15/18/21/24 words. | Encoding width is not measured randomness. |
| Coins | Strict raw `0`/`1`; Heads = 0 and Tails = 1, MSB first. | No whitespace, alternate alphabet, or hash conditioning. |
| D6 raw | Records `1`–`6` and hashes the roll transcript with SHA-256. | Hashing biased or predictable rolls does not make them random. |
| D6 Coleman | Maps `6` to `0`, then hashes the transcript with SHA-256. | It is a distinct transcript from raw D6; the device cannot test die fairness. |
| Words | Imports and validates 12/15/18/21/24 canonical English BIP39 words. | Dictionary lookup and checksum validation add no entropy. |
| Seed | Imports BIP39 word-list numbers in 1–2048 or 0–2047 form and helps select a checksum-valid final candidate. | Number preview and checksum filtering are deterministic. |
| Cards | Implements direct rank phases A–8, A–4, and A–2. Suits and deck permutations are not implemented. | Shuffle and draw quality remain external assumptions. |
| Bases | Encodes externally produced bits using base 4, 8, 32, or 64 controls. | Base conversion is representation, not an entropy source. |
| BitBox-style dice | Five D4 outcomes made with D6 rejection, then one coin-like outcome per prefix word, followed by explicit final-candidate choice. | No BitBox affiliation or endorsement; fair independent physical outcomes are still required. |
| D++ D8/D16 | Directly maps one D8 and two D16 positions per prefix word, with size-specific final controls. | The firmware defines this screen; no independent public D++ specification was established. |

The output pipeline uses a Rust `no_std` + `alloc` runtime with bounded C ABI buffers. It computes the mnemonic, BIP32 master fingerprint, and only the first mainnet BIP84 receive address at `m/84'/0'/0'/0/0`, using the empty BIP39 passphrase unless the explicitly implemented passphrase flow is used. Editing invalidates previous results. Clear is practical UI cleanup, not proof of complete secret erasure from RAM, stacks, queues, allocations, or library temporaries.

The application has no signing or transaction workflow. Its current application paths do not intentionally use networking or persistent secret storage. The board nevertheless contains an ESP32-C6 wireless coprocessor, so “offline application” is not the same as a proven physical air gap.


## Historical evidence

The [original source-candidate landing page](docs/HISTORICAL-README.md) preserves earlier branch, build and acceptance claims, not current guidance. No historical hashes or receipts have been rebaselined.
