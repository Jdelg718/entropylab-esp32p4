# Supported destructive first-install contract

Review candidate; publication requires independent review. This is NOT the retention updater.
Only Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 PCB rev1.3, 32 MiB is supported.
Physically inspect the PCB marking. Browser chip checks cannot establish PCB identity.
Unknown boards, revisions and profiles must not be accepted by assumption.

## Run locally

Install Python 3. Either clone this repository with Git, or verify the release ZIP's
published SHA-256 and extract it (Git is not required for the ZIP). Enter the
repository or extracted directory, then:

- Linux/macOS: `python3 scripts/serve-public.py`
- Windows: `py -3 scripts/serve-public.py`

This loopback-only server allows tracked public files only (using the packaged
`release/inventory.json` allowlist when Git metadata is absent); Git metadata,
directory listings and added backups are refused. Use a clean clone or verified
release ZIP, and do not edit its allowlist. Native Windows
and macOS execution has not been independently tested.

Open http://localhost:8000/flash/first-install/ in desktop Chrome or Edge with Web Serial.
Use localhost or HTTPS, not file URLs or an insecure remote HTTP host. Safari/Firefox
are not supported. Use a USB data cable and the board's programming USB connection;
close serial monitors. Linux may require distribution-specific serial-device group
permission (sign out/in after changing membership); do not run your browser as root.
Windows may require the manufacturer's USB serial driver. No Omarchy-specific setup.

## Export and backup BEFORE replacing factory firmware

The browser does not create a backup. Export settings/data using your existing
firmware's export facility if available. Separately read the entire 32 MiB flash
with a trusted local esptool installation before writing, using the port you identify:

`python -m esptool --chip esp32p4 --port PORT read-flash 0 0x2000000 factory-backup.bin`

Use `python3` on Linux/macOS or `py -3` on Windows as appropriate. Install esptool
in a Python virtual environment following its official instructions. Hash the saved
file with SHA-256, read it again and compare hashes, and store both backup and digest
securely offline. Never upload backups: they can contain credentials and identifiers.
Readback is not proof restoration works. Secure boot/encrypted flash boards are refused;
stop rather than bypassing guards. If preservation matters and you cannot establish a
recovery procedure, do not install. No project-provided private backup belongs to you.

## Installation

Verify images, check board confirmation, select the correct port and explicitly
consent to destructive replacement. A diagnostic is optional but consumes the session;
reload afterward. Click Replace factory firmware only when ready. Factory demo,
settings and overlapping provisioning/NVS/OTA/PHY contents are destroyed. Keep power
and cable stable. No automatic retry, whole-chip erase, backup or success reset occurs.
Wait for all three images and erased tails to verify, then press RESET. Check display,
touch, the public D6 test, Back, and reopen Test results without recalculating.
On failure, save only redacted status, disconnect if cleanup is uncertain, and investigate
before a fresh attempt. A verified write does not guarantee boot or recoverability.

## Policy and evidence boundary

The public profile explicitly replaces private predecessor-hash authorization with
physical supported-board confirmation and destructive consent. It does not authenticate
an individual board or claim existing factory contents match any known device.
Exact profile equality, pinned image hashes, chip ID 18, silicon revision 100–199,
ROM ECO 0 or 2, known security flags, secure-boot/download disabled, zero encryption
counter, and raw JEDEC 32 MiB density remain required. PCB rev1.3 is a separate human
check, not the silicon revision. Other values fail closed. Writes preserve image header
parameters (`keep` size/mode/frequency), compress, never erase all, and verify image and
FF-tail SHA-256 readbacks. Tool-unwritten regions are not asserted physically unchanged.

The exact three-image tuple has prior browser-flash/readback and human boot/display/
touch/D6 Back-retention acceptance on one fresh board. The new public policy itself has
not been exercised on hardware. Independent native whole-flash comparison did not finish:
no outside-window equality, preboot continuity, broad application qualification or tested
restoration claim. The existing retention updater and its release hold are unchanged.

## Sources and pins

Accepted source archive: `release/retention-public-source.tar.gz`, SHA-256
`82d62829596fe080f6bf02cf67c3337d88d4287a98e6b82ad3c70aa56f8e4f19`.
Extract separately; validate with `python3 verify-public.py --anchor
3110081933447e5cb0e7e7d2e326a089701b2cba74e4a1341a5152f66765a7e9` from its root
(on one command line). It preserves sdkconfig, provenance, identity and license records.
Root source imports that accepted successor; historical build checks may reject it.
This release is not a reproducible firmware-build claim; no new firmware was built.

| Image | Offset | SHA-256 |
|---|---|---|
| bootloader.bin | 0x2000 | 530330004af627bf2de7ac9b1d05ab34f8a655829a74005b96fc15dd7083f678 |
| partition-table.bin | 0x8000 | d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb |
| entropylab.bin | 0x10000 | a02a2192b5c302390a916f9ee67961b699e5f2f9038135378eed0b19d792f33b |

Images are under `flash/first-install/firmware/`. Complete notices are retained alongside
both browser installers and in source; do not remove or replace upstream licenses.
