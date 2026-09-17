# EntropyLab for ESP32-P4 (unofficial)

## Public UI preview quick start

**This branch is a public-source UI preview, not a hosted web installer or
installable firmware. It cannot connect to or flash a fresh board, and it provides
no firmware downloads. No device is needed. Hardware connection, flashing,
downloads, release and feedback submission are disabled. This source preview does
not lift the firmware publication HOLD.**

Prerequisites: **Git**, **Python 3** and a **modern desktop browser**. Install Git
and Python 3 first if they are not already available in your terminal.

Clone this branch and start the local preview (Linux/macOS command example):

```sh
git clone --branch preview/installer-ui --single-branch https://github.com/Jdelg718/entropylab-esp32p4.git entropylab-ui-preview
cd entropylab-ui-preview
python3 scripts/serve-flash-ui.py --port 8765
```

On Windows, use the same clone and `cd` commands. If Python 3 is installed with
the Windows Python launcher (`py`), replace the server command with:

```powershell
py -3 scripts/serve-flash-ui.py --port 8765
```

Open **http://127.0.0.1:8765/flash/** in your desktop browser. Leave the terminal
running; press **Ctrl+C** there to stop. After cloning, running the preview needs
only **Python 3** and a modern browser: no Node, Rust, ESP-IDF, firmware build,
board, USB cable or driver. The server binds only to loopback and serves five
allowlisted UI files, never firmware or directory listings. The page makes no
external requests; its strict CSP blocks connections. Do not expose this preview
through a public proxy. If the port is busy, choose another `--port` and use the
same port in the browser URL.

### What to try (no device required)

- Switch between **Install** (three-image explanation) and **Update** (app-only
  explanation). These are UI states, not working installation paths.
- Exercise the board/practice confirmations as a simulation and inspect the
  **data-loss consent**. Changing mode clears that consent. Checking every box
  still cannot enable hardware; these checkboxes are not device validation.
- Expand **Source & image integrity**. It shows an exact metadata-only snapshot
  of an integrity-verified, **unsigned private first-install candidate**, with its
  original source commit/tree and manifest/image hashes. The public preview commit
  is not that firmware's build identity. The snapshot remains **NOT APPROVED FOR
  FLASHING**, built before merge, not rebuilt from merge, not reproducibly verified,
  and first-install unexercised. Hash agreement is not authenticity or approval.
  No BIN, package, private manifest file or firmware download is included here.
- Expand **Feedback stays in your hands** and opt in to a fixed-choice local
  preview. Opt out or reload to discard it. There is no free-text capture, storage,
  clipboard access, telemetry or submission; Share remains disabled.
- Verify **Preflight BLOCKED**, with transfer, readback and boot confirmation
  separately **PENDING**. Connect, release/download and sharing stay disabled even
  after all confirmations. Do not connect hardware for this preview.

Never enter real seeds, mnemonics, passphrases, private keys or funded-wallet data.
Mobile/unsupported-browser banners describe future installation constraints, not
a claim that this UI can flash in another browser. The macOS and Windows commands
are untested examples, not exercised platform acceptance. Linux command checks
and automated Linux Chromium UI tests are not OS or hardware acceptance.

### Optional contributor tests (not needed to run the preview)

```sh
node --test tests/flash-ui/model.test.mjs
python3 -m venv .venv
.venv/bin/python -m pip install playwright
.venv/bin/python -m playwright install chromium
.venv/bin/python tests/flash-ui/browser.py
```

The browser test uses `FLASH_UI_CHROME` when set to a browser executable, otherwise
an installed Chromium/Chrome or Playwright's Chromium. `FLASH_UI_EVIDENCE` can name
an output directory outside the checkout; by default evidence goes into a new
system temporary directory. These developer-only dependencies are separate from
the Python-standard-library server. Tests exercise real loopback HTTP, UI behavior,
security denials and responsive layouts; they do not test a hardware driver.

### Firmware documentation below — separate scope

The original native-firmware documentation, release HOLDs, historical evidence,
licenses and provenance below are retained. Their candidate/branch identifiers
refer to the native-firmware work, **not this UI-preview branch or a firmware
release**. No merge, deployment, release, binary redistribution or hardware
acceptance is implied. The preview adds no third-party runtime assets or fonts;
existing [provenance](docs/PROVENANCE.md) and [license notices](THIRD_PARTY_NOTICES.md)
remain unchanged. Credit **EntropyLab — Team Ooga Booga** and the original
[upstream project](https://github.com/OogaBoogaX/entropylab). Preserve the exact
custom Ooga Booga license and all third-party notices; this is not a blanket MIT
license grant. Full firmware binary redistribution/license review remains open.

---

This unofficial adaptation is independently maintained native software, not the upstream HTML product; it is not shipped, audited, or endorsed by OogaBoogaX. Upstream naming guidance concerned a historical empty-passphrase milestone, not security review or endorsement of this unreleased extension. The implemented optional BIP39 passphrase flow and D6 transcript editing extend that milestone; they are not an empty-passphrase-only claim. Public practice only: entropy is user-supplied, not generated or certified by the application.

EntropyLab is an experimental, offline-first educational application for entering public practice entropy, deriving BIP39 English mnemonics, and displaying a master fingerprint, first BIP84 receive address, and LifeHash-style visual fingerprint. It is intended to make input assumptions, encoding rules, checksum behavior, and common mistakes visible on a touchscreen.

Contributors are welcome. Start with [CONTRIBUTING.md](CONTRIBUTING.md), then read the gated [build and flash guide](docs/BUILD-FLASH.md).

> **Public practice only. Never enter a real seed, mnemonic, passphrase, private key, or funded-wallet material.** This project is experimental. It is not a wallet, signer, transaction device, audited cryptographic product, certified random-number generator, or secure-erasure implementation. A valid checksum, full input width, hash, or attractive fingerprint does not prove that physical input was fair or unpredictable.

## Current publication status

**Unreleased source candidate; bounded source review and complete fresh host suite passed. Publication remains on hold.** This candidate adds numbered D6 transcript review and individual roll correction. It is not the currently installed firmware and has no approved downloadable firmware or browser installer. No device acceptance or reproducible-build claim is made. See [publication status](docs/PUBLICATION-STATUS.md) and [current source mapping](docs/MODAL-COMBINED-SOURCE-MAPPING.json).

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

## Supported hardware

The hardware profile, historically exercised with predecessor firmware, is the **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**, portrait **480 × 800**, with a physical ESP32-P4 **Rev1.3** device. The target profile selects ESP-IDF's Rev1.x compatibility range (`CONFIG_ESP32P4_REV_MIN_100`) and 200 MHz PSRAM; the historical accepted image reports a supported silicon revision range of 1.0 through 1.99. This is not a compatibility promise for Rev3.x boards or similarly named displays.

Official Waveshare identifiers:

- SKU 33874: `ESP32-P4-WIFI6-Touch-LCD-4.3`, standard version without camera.
- SKU 33875: `ESP32-P4-WIFI6-Touch-LCD-4.3-C`, version with optional OV5647 camera.
- 4.3-inch IPS, 480 × 800, ST7701 over 2-lane MIPI-DSI, GT911 capacitive touch.
- ESP32-P4NRW32, 32 MB in-package PSRAM, 32 MB external NOR flash, and onboard ESP32-C6-MINI-1.
- Separate USB-to-UART and USB OTG Type-C ports, BOOT and RESET buttons, TF/microSD slot, MIPI-CSI camera connector, speaker header, battery headers, and 40-pin expansion header.

Some sellers call bundles “Package C.” That seller label is not a board-revision identifier. The official product distinction is the `-C` SKU with the optional camera. Verify the actual board/silicon revision before building or flashing. The camera, included 8 Ω 2 W speaker, batteries, TF card, and 40-pin adapter are not required by EntropyLab. A known-good **USB data** cable is required and is not listed in Waveshare's official quick package overview.

For programming/debugging, use the Type-C port labeled **USB TO UART**, not the adjacent USB OTG port. The official connector and accessory evidence is collected in [docs/HARDWARE.md](docs/HARDWARE.md).

## Build and flash

The source pins:

- repository branch candidate: `release/d6-review-edit-20260917` (not remote until reviewed and pushed);
- ESP-IDF v5.5.5 commit `b774170ff46c393eeb5e495ea37936038d3f4f4f`;
- host Rust 1.95.0;
- target Rust `nightly-2026-04-15` with `rust-src`;
- LVGL v9.5.0 commit `85aa60d18b3d5e5588d7b247abf90198f07c8a63` for host GUI tests;
- BSP/LVGL and transitive ESP-IDF components pinned by `fixture-firmware/app/dependencies.lock`.

Use [docs/BUILD-FLASH.md](docs/BUILD-FLASH.md) for the build procedure and gated install/recovery requirements. Complete successor firmware assets, first-install/update/recovery acceptance and hardware smoke verification remain pending.

The documented target-build entry point never flashes. A complete target image for this successor has not been built or verified:

```sh
bash scripts/build-firmware.sh
```

The complete `bash scripts/test-host.sh` gate passed (inner exit 0) on a fresh source-only copy of combined manifest `0c11e6fd9fcccc260f78918367c588c5095c755382cad35920a5c88a3ff7de10`. It compiled fresh host artifacts, ran the default GUI and all ten pointer-family slices, and continued through the combined native suite. Rust 1.95.0, GCC 14.2.0 and CMake 3.31.6 were inspected; existing LVGL sources and an isolated copy of the dependency registry were reused. This is clean-source/build-directory host assurance, not a fresh-network dependency test, ESP32-P4 build, binary reproducibility certification or hardware acceptance. Upstream dependency compiler warnings were retained, not represented as warning-free success.

The same combined source received bounded independent source review: 37 focused tests and 74 independent semantic probes (three accepted valid controls, 71 rejected invalid mutations). All original findings are closed for that reviewed code. This package changes documentation/status only after those gates; `HOST-VERIFICATION.json` binds the tested manifest and stream hashes. Production, tests, scripts, lockfiles and historical successor manifests remain byte-identical to the tested package.

Complete target image/ELF/map/resource/layout verification, exact pushed-SHA CI, first-install/update/recovery tests, physical acceptance and release/flasher review remain separate pending gates. No release, download URL, installed identity or publication approval is conveyed.

Historical note: the predecessor failed its D6 pointer-navigation test after compilation; this combined successor includes the reviewed repair. See [status/history](docs/PUBLICATION-STATUS.md).

Do not invent flash offsets or reuse binaries from another build directory. The generated `flasher_args.json` is the authority for that build.

## Contributing

Good first contributions include:

- public-fixture tests and regression cases;
- clearer educational explanations and accessibility improvements;
- portable layout work for future screen sizes;
- separately scoped board-support investigations;
- build/documentation improvements that remain reproducible from public sources.

Never submit real secrets, device dumps, device identifiers, private paths, credentials, or unsanitized serial logs/screenshots. Mockups are welcome, but must be labeled as mockups rather than firmware behavior. See [CONTRIBUTING.md](CONTRIBUTING.md).

## Roadmap

Support for other screen sizes and hardware devices is planned, not implemented or supported today. The first portability work is to separate layout metrics, board support, display/touch adapters, and hardware acceptance tests. There is no promised schedule or compatibility matrix. See [ROADMAP.md](ROADMAP.md).

## Provenance and licenses

This is selective reuse, not a full fork of [OogaBoogaX/entropylab](https://github.com/OogaBoogaX/entropylab). The pinned upstream commit and all adapted sources, fonts, vectors, artwork, and third-party components are documented in [docs/PROVENANCE.md](docs/PROVENANCE.md), [docs/EDUCATION-PROVENANCE.md](docs/EDUCATION-PROVENANCE.md), and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Retain all adjacent license files. No upstream, hardware vendor, or named method provider endorses this adaptation.

