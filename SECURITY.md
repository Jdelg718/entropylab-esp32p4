> **Current guidance:** [SECURITY — current retention preview](docs/current/SECURITY.md).
> The text below is the historical source checkpoint, not current release status.
> v0.1.0-retention-preview is published; v0.1.1 is a local documentation/onboarding patch pending publication.
> Firmware is unchanged; public-policy hardware qualification and the separate retention updater remain held.

# Security boundary

This unofficial adaptation is independently maintained native software, not the upstream HTML product; it is not shipped, audited, or endorsed by OogaBoogaX. Upstream naming guidance concerned a historical empty-passphrase milestone, not security review or endorsement of this unreleased extension. The implemented optional BIP39 passphrase flow and D6 transcript editing extend that milestone; they are not an empty-passphrase-only claim. Public practice only: entropy is user-supplied, not generated or certified by the application.

The supported board profile is Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3. Outputs are the BIP39 English mnemonic, master fingerprint and only the first mainnet BIP84 receive address at `m/84'/0'/0'/0/0`. Empty passphrase is the default; the explicit optional passphrase flow is implemented but unreleased. The board contains an ESP32-C6 wireless coprocessor: an offline application is not a proven physical air gap.

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
