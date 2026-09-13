# Contributing

Use public test vectors only. Read SECURITY.md and AGENTS.md first. Run
`bash scripts/test-host.sh`; record actual command outcomes, toolchain versions,
and whether hardware was involved. Keep changes small and scoped. Preserve locked
dependencies and upstream licensing; explain extraction modifications explicitly.
Never add real seeds, flash dumps, device identifiers, private paths or raw logs.
Do not enable signing, persistence, networking or random-key generation without
separate approval. UI design contributions must distinguish mockups from firmware.

Before proposing dependency upgrades, review license/security changes and rerun
host, target ABI/configuration checks and a separately authorized hardware test.
Report failures honestly; do not replace failing vectors with updated expectations.
Publication and contributor outreach are separate owner-approved actions.
