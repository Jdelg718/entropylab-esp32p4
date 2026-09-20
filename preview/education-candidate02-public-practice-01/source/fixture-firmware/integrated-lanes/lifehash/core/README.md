# REVISION-05 LOCAL ADAPTATION — independent re-review required

See ADAPTATION.md. Any pristine-vendor statements below describe the historical base only, not this revision. Binary notices must accompany distributed binaries.

# EntropyLab LifeHash fingerprint module

A pure native wrapper around the pinned Blockchain Commons C++ reference implementation. It accepts exactly four raw BIP32 master-fingerprint bytes and produces exactly 32×32 RGB888 (3,072 bytes) using LifeHash version2, module size 1, no alpha, and `make_from_data` hashing semantics.

The API deliberately exposes no mnemonic, passphrase, seed, xpub, text/hex parser, digest path, key state, adjustable dimensions, module size, alpha mode, or alternate LifeHash mode. A version discriminator is mandatory and every value except product constant `2` is rejected. Invalid calls and exceptions leave caller output untouched.

Run on the prepared Linux validation lane:

    bash run-tests.sh

The script is offline after vendoring, builds only this subtree with CMake/g++, writes generated files under `build/` and `evidence/`, runs boundary/golden/negative/determinism/input-and-output-canary tests, byte-compares actual pixels with an upstream vector, creates a real PPM render, measures one normal Linux host invocation with Python `resource.getrusage`, and runs ASan/UBSan when supported.

LifeHash is a visual aid only. It is not identity proof, security proof, or an entropy test. Host measurements are not ESP32-P4 measurements. Target timing, stack, heap/PSRAM, linked flash, watchdog, and UI responsiveness remain unknown until separately measured on approved hardware.
