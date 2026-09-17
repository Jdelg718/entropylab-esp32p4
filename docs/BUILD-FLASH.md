# Build, install and recovery boundary

**This is an unreleased source candidate, not a tested first-install kit. No firmware assets or safe first-install command are available from this preparation.** Build instructions are in [BUILD.md](BUILD.md). They pin ESP-IDF v5.5.5 commit `b774170ff46c393eeb5e495ea37936038d3f4f4f`, Rust host 1.95.0, target nightly-2026-04-15 plus rust-src, and LVGL 9.5.0 commit `85aa60d18b3d5e5588d7b247abf90198f07c8a63`. Cargo and component lockfiles are retained. Do not use an unpushed branch checkout command: the proposed branch is not a download location.

## Board identification

Only Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 (480 × 800 portrait) is in scope. Historical hardware evidence is for ESP32-P4 silicon Rev1.3; this profile permits Rev1.x, not Rev3.x. Silicon identity alone does not establish board model or board PCB revision. Compare the actual product and connector labels with the [official board photos/documentation](https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-4.3) and [hardware guide](HARDWARE.md). No newly taken board photo or accepted PCB-revision matrix is claimed.

Use the **USB TO UART** Type-C connector and a known-good data cable, not USB OTG. Camera, speaker, battery and microSD are not required. Linux needs permission to access the enumerated serial port; use your distribution's existing group/udev policy. Do not run tools as root as a workaround. Close other serial applications. For manual download mode, hold BOOT, press RESET once, then release BOOT; verify this against Waveshare's board instructions. These steps are guidance, not hardware validation of this candidate.

## Required asset sets and verified historical offsets

The following values were read from generated ESP-IDF flash metadata and checked against its partition-table binary. They are evidence for the predecessor layout, NOT an approved successor installation manifest. `FLASH-LAYOUT-EVIDENCE.json` binds the inspected metadata and asset hashes. The candidate retains the same partition CSV/configuration. The integration owner must independently confirm the successor's generated metadata and bind it to reviewed source before any release.

| Asset | Historical offset | First install | Compatible app-only update |
|---|---|---|---|
| `bootloader/bootloader.bin` | `0x2000` | Required, from same approved build | Do not overwrite |
| `partition_table/partition-table.bin` | `0x8000` | Required, from same approved build | Do not overwrite |
| `entropylab_fixture.bin` | `0x10000` | Required, from same approved build | Required, only after layout/bootloader compatibility verification |

Metadata settings: ESP32-P4, DIO, 32 MB flash, 80 MHz flash clock. Custom partitions: NVS at 0x9000, PHY data at 0xf000, factory application at 0x10000 (8 MiB), storage at 0x810000 (7 MiB). These NVS/PHY/storage partitions have no separate programmed asset in the inspected flash manifest. ELF and map are provenance/debug artifacts, not flash payloads. Include checksums, exact source commit, build/toolchain manifest, all flash payloads, generated offsets and a manual guide in any future release ZIP. Do not mix builds or substitute a factory backup, C6 image or unrelated bootloader.

First installation on factory/unknown firmware must be separately tested with the complete approved set. App-only update is not a first-install shortcut: verify the existing bootloader, partition map, silicon revision and application limits against the new build. Unknown compatibility means STOP, not try/force. Flashing overwrites bytes and may destroy prior firmware/data; it is not secure erasure. No whole-chip erase, eFuse/security setting change, companion-C6 flash or hidden destructive recovery is authorized or needed by this guide.

## Why there is no write command here

The exact successor full asset set and first-install hardware path are not yet validated. A plausible esptool command with copied offsets would overstate readiness. After those gates pass, a separately reviewed owner-facing guide must use the released immutable manifest or that build's own ESP-IDF generated flash arguments, take explicit consent, and distinguish full installation from app-only update. No placeholder port or unpublished firmware URL is presented as a final instruction.

## Post-install acceptance (required, not executed here)

Verify the exact downloaded/written application hash, boot identity and usable portrait touchscreen. With public deterministic fixtures only, exercise all input methods, Safety/About, Clear, navigation/scroll, saver/wake and practical responsiveness. For D6, review 1/100/1024 rolls, edit selected rolls preserving all others and the count, recompute, reject stale results, and confirm RAW versus Coleman isolation. Never collect real rolls, mnemonics, passphrases, fingerprints, seed screenshots or raw serial logs.

See [RECOVERY.md](RECOVERY.md) and [KNOWN-LIMITS.md](KNOWN-LIMITS.md). Firmware release, manual flashing validation and browser flasher acceptance are separate from source publication.
