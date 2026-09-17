# Security boundary

DEVELOPMENT TEST ONLY. Unofficial. Not an OogaBoogaX product, audit, or endorsement.
No production security support or secret-safety guarantees.
Never use real funds, mnemonic phrases, passphrases or private keys. The compiled
fixture is publicly known. Never fund any displayed address.

Native contract today:
- user-supplied entropy only; the device must not invent secret bits
- empty BIP39 passphrase only
- first mainnet BIP84 receive address only (`m/84'/0'/0'/0/0`)
- a BIP39 checksum or a SHA256 transcript is not entropy quality

The ESP32-P4-Function-EV-Board still includes an ESP32-C6 radio. Offline firmware
is not a physical air gap. Do not call this air-gapped hardware.

The ABI bounds outputs but cannot validate arbitrary dangling C pointers. Allocation
failure and panic abort. Secret erasure is not complete. Host tests are not an audit.
No application networking/signing/storage/RNG initialization is intended; SDK
support routines may still be linked.

Do not post secrets, device dumps or exploitable private deployment details in public
issues. Before publication, a private reporting contact has not been established;
request a reporting channel without including sensitive details. No response-time
or vulnerability-bounty commitment is made. Independent review is a release gate.
