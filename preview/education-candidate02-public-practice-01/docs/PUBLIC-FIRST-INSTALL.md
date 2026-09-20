# EntropyLab education-candidate02-dev-test-01

LOCAL DEV TEST ONLY. Public distribution HOLD; **diagnostic and install are enabled**.
Not a production release or hardware-accepted candidate. Never use real secrets.

Firmware is unchanged candidate02, source `2b919dc73c9cbcbb5e845850650d71c6782ad68b`, recipe `0ec7f6dd8ec62d6d554fea6726deeeb5a795a102`.
Static compatibility against the accepted EL-002 tuple passed: see compatibility.json.
Generated app and bootloader configurations are identical; image chip/revision and flash
settings match; partition table bytes and flash offsets match. New app erase end is
1609728 (one extra sector inside the same application partition).
No app-only or prior-content authentication claim: this is the explicit three-image
replacement route, preserving all bytes outside its sector-rounded write windows by
command scope, not by a full-chip readback proof.

## Operator steps — one attempt, preserved backup
1. Keep the existing verified dev-board backup and recovery records outside this served
   directory. This installer does not make a backup. Do not proceed if that backup is missing.
2. Extract into a new directory. Run `python3 verify-package.py .`.
3. Serve only this extraction locally: `python3 -m http.server 8765 --bind 127.0.0.1`.
   Open `http://127.0.0.1:8765/flash/first-install/` in desktop Chrome/Chromium.
4. Confirm Waveshare P4 4.3 PCB rev1.3 / 32 MiB, Verify three images, select the
   USB-to-UART port, and run the no-write diagnostic. It loads the official RAM stub.
   ROM ECO must be 0 or 2; chip 18 / revision 100–199; secure boot/download off;
   encryption count zero; raw JEDEC density 32 MiB. Any refusal means STOP.
5. After diagnostic completion, reload, verify, select again, and explicitly check board
   and preserved-backup/destructive consent. Click **Install DEV TEST firmware** once.
   Expected: 3 image writes and SHA-256 verification of 3 images + 3 all-FF tails.
   Baud changes 115200 → 460800 only after security and official stub checks.
6. Only after verified completion, manually RESET. Check boot, touch, public 24-word
   fixture/fingerprint, education pages, Back and retained Test results. These are tests
   to perform, not claims already established for candidate02.
7. On failure/cancel/unplug, STOP. No automatic retry, rollback, erase-all, eFuse change,
   security bypass or C6 write. Preserve the finite failure report for review before
   authorizing another attempt. Do not press reset as a substitute for readback success.

## Sources, notices and assurance
Complete tracked corresponding firmware/core sources and exact original notices are
under source/; installer/vendor notices and root licenses are retained unchanged.
External dependencies remain lock-pinned, not vendored. Public distribution licensing,
independent reproducibility, exact-candidate physical acceptance, recovery qualification
and publication approval remain open; none is represented as satisfied by this DEV TEST.
Tests model transport and public memory, never private backups. Hashes establish local
byte consistency, not authenticity. No firmware rebuild was performed.
