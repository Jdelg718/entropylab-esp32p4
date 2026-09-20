# EntropyLab — unofficial ESP32-P4

## Education candidate02 — published public-practice prerelease

**Public practice only, not a wallet or signer. Never enter real seeds, passphrases,
private keys or other secrets; never fund displayed addresses.** This is an
unofficial native adaptation of EntropyLab — Team Ooga Booga, not endorsed by
OogaBoogaX, Waveshare or BitBox.

This checkout includes the installable candidate02 firmware and animated flasher
in an additive versioned directory. The earlier v0.1.0/v0.1.1 retention-preview
releases, images and historical pins are unchanged. This is **new education
firmware**, not the old documentation-only patch. No rebuild was performed.

**Experimental public-practice prerelease; user-tested on one board only.**
The finite notice disposition is closed. Candidate02 is published on main and as the
[public-practice prerelease](https://github.com/Jdelg718/entropylab-esp32p4/releases/tag/education-candidate02-public-practice-01).
See
[status and provenance](docs/PUBLICATION-CANDIDATE02.md) and
[exact source and build route](docs/SOURCE-AND-BUILD.md).

## Get and run locally

Requires Python 3 and desktop Chrome with Web Serial (Chromium-based Edge may
also work). No Node, Rust, ESP-IDF, package installation or private build checkout
is required to run the installer.

For the published successor, clone main:

```sh
git clone https://github.com/Jdelg718/entropylab-esp32p4.git
cd entropylab-esp32p4
```

Alternatively choose **Code → Download ZIP** on GitHub, extract it completely,
and open a terminal in the extracted `entropylab-esp32p4-main` folder (the folder
containing this README). GitHub main includes the candidate02 installer and firmware.

Linux/macOS:

```sh
python3 scripts/verify-publication.py
python3 scripts/serve-public.py
```

Windows (PowerShell or Command Prompt):

```powershell
py -3 scripts/verify-publication.py
py -3 scripts/serve-public.py
```

Open **http://localhost:8000/** in desktop Chrome. The loopback server routes to
`preview/education-candidate02-public-practice-01/flash/first-install/`, not the historical
root flasher. Do not open the HTML with `file://` or expose this server publicly.
Use Ctrl+C in the terminal to stop. Linux clean clone/ZIP and Chromium checks are
reported separately from native macOS/Windows execution, which remains untested.

## Board and installation

Only **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, PCB rev1.3, 32 MiB flash**, is in
scope. The chip identifier/revision alone does not establish the board model.
The installer checks ESP32-P4 chip 18, revision 100–199, ROM ECO 0 or 2, raw JEDEC
32 MiB and disabled secure boot/encryption/download-security restrictions. Do not
bypass any refusal, and do not use this for a different board or its C6 coprocessor.

1. Preserve a verified board backup and recovery instructions **outside this
   checkout**. The installer does not create a backup. Stop if unavailable.
2. Connect the USB-to-UART port with a data-capable cable. Close serial monitors.
3. Verify the three images, confirm the exact board and select its serial port.
   Run the no-write diagnostic first; it loads the official RAM stub.
4. Reload, verify/select again, then explicitly accept the preserved-backup and
   destructive overwrite consent. Click **Install DEV TEST firmware** once.
   This replaces three image sector windows, including erased tails; it is not
   an app-only update. No erase-all, C6 write, automatic retry or eFuse write.
5. Wait for verification of all three images **and all three erased tails**.
   Only after successful completion press **RESET**, then check display, touch,
   education pages, a public fixture/fingerprint, Back and retained Test results.
6. On failure, cancel or unplug: **STOP**. Save the finite failure report, removing
   identifying details before sharing; seek review rather than retrying blindly.

Read the [public-practice package instructions](preview/education-candidate02-public-practice-01/README.md)
for the exact operator contract. Its earlier NOT TESTED labels are historical;
the subsequent one-board user report is recorded separately, never retroactively
rewritten into its immutable manifest.

## Changes and public testing

Education text now explains biased dice/coins, independence, transcript hashing,
representation versus entropy and public worked examples. The flasher adds
per-image live progress and a guarded 115200 → 460800 baud transition. Candidate02
uses the hardened build recipe and private runtime heap checks; these are not a
claim of independently reproducible builds or entropy-source certification.

The exact dev package was installed by its owner on **one board**: installation
reported three images plus three erased tails verified, followed by the owner's
“everything works” report after RESET/boot/touch/education/retention checks were
requested. This is user-reported acceptance, not an exhaustive instrumented
hardware campaign. Other boards, recovery qualification, independent rebuilds
and native Windows/macOS testing remain unqualified.

**BitBox-style dice is not proven interoperability with BitBox hardware.** The
maintainer has no BitBox device. BitBox owners are invited to compare **public,
disposable known vectors only** and report model/firmware, exact public dice/coin
transcript, word-count/final-candidate choices and expected versus actual output.
Never submit a real wallet seed, private key, secret passphrase or private backup.
Please report OS/browser and sanitized errors for installation tests as well.

## Troubleshooting

- No serial chooser: use desktop Chrome on localhost, not mobile Safari or file://;
  close another tab/serial monitor holding the port and check OS serial permissions.
- No board/port: check USB-to-UART connector and a known data cable; consult the
  board vendor's driver/bootloader instructions. Do not disable security checks.
- Port 8000 busy: stop the previous preview server, then rerun the same command.
- Hash mismatch: stop and obtain a fresh complete checkout/archive; do not edit pins.
- Verified install but no boot: use manual RESET once after success, record symptoms;
  do not reinstall as a substitute for checking boot or recovery.

## Source and license scope

[Third-party notices](THIRD_PARTY_NOTICES.md), the exact custom Ooga Booga license,
MIT/BSD/OFL notices and dependency terms remain intact. The versioned package
includes the corresponding project sources and collected exact dependency notices.
External dependencies remain pinned but not completely vendored. The bounded notice determination is recorded in the publication status; it is
not legal certification. Root `flash/` paths are legacy, not candidate02 onboarding.

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
