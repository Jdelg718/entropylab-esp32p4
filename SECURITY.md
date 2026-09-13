# Security boundary

DEVELOPMENT TEST ONLY. No production security support or secret-safety guarantees.
Never use real funds, mnemonic phrases, passphrases or private keys. The compiled
fixture is publicly known. Never fund any displayed address.

The ABI bounds outputs but cannot validate arbitrary dangling C pointers. Allocation
failure and panic abort. Secret erasure is not complete. Host tests are not an audit.
No application networking/signing/storage/RNG initialization is intended; SDK
support routines may still be linked. This is not an air-gap certification.

Do not post secrets, device dumps or exploitable private deployment details in public
issues. Before publication, a private reporting contact has not been established;
request a reporting channel without including sensitive details. No response-time
or vulnerability-bounty commitment is made. Independent review is a release gate.
