# About this input — compact explanations

A bounded native UI milestone following Words/Orbit/practical cleanup at
`47f19d983b493052335a98a1990b3d0a2eea0e02`. No new entropy input, derivation,
passphrase, score or storage capability is introduced.

## User flow

Select an input mode, open **Public test only / Safety**, then **About this input**.
The explanation is specific to the selected mode. **Back to Safety** restores the
original Safety/About page; **Close** returns to the unchanged underlying screen.
A new Safety visit starts on Safety, not the previous explanation.

| Mode | Explanation |
| --- | --- |
| HEX | Encoded bit width is not measured randomness; a full-length value can be predictable. |
| Coins | Heads=0, Tails=1, MSB-first packing, exact count, no input hash/padding/truncation; preview is not the whole retained input. |
| D6 raw | Hash every recorded ASCII roll in order with SHA-256, then use the selected prefix. Nominal counts assume fair independent dice. |
| D6 6-to-0 | Map each 6 to ASCII 0 before hashing every roll; distinct from raw D6. Hashing cannot create missing entropy. |
| Words | English dictionary, count and checksum validation do not establish randomness quality or wallet ownership. |

Existing styling, input panels, counters, result cautions, Safety text and credits
remain intact. There are no repeated new warning banners. Explanation text is
static: no transcript, mnemonic, fingerprint, address or live user value is copied
into it. These explanations are not a statistical test or certification.

## Behavioral contract

- Modal navigation does not submit work, clear input or invalidate existing results.
- Background controls remain blocked throughout both pages.
- Page transitions reject stale callbacks and retain existing generation guards.
- Busy/confirmation exclusions and the Words saver's first-contact consumption
  remain unchanged. The saver is not a lock or wipe.
- Close stays reachable outside the explanation's vertical scroll area.
- No new dependencies, runtime ABI, input mapping or cryptographic changes.

## Verification boundary

Verification completed for this local candidate:

- Final full host suite passed with stable source/test hashes, including all five
  explanation pages, state/scroll preservation, stale callbacks, busy/confirmation,
  saver wake and recreation. The generation-exhaustion test uses documented
  synthetic boundary injection and real LVGL events; it is not a claim of billions
  of physical interactions. One pre-existing optional upstream drift check skips
  when its external checkout is unavailable.
- Fresh source-built ESP32-P4 firmware passed revision/configuration, runtime
  ABI/API/linkage and strict main/gui compilation gates without production drift.
- Iris approved actual native renders for all five modes and intact Safety copy.
  Rex's independent bounded code/security review found no blockers.
- Parent reconciled the later exhaustion-test addition and required provenance
  pins, verified final host hashes, and reran the provenance verifier plus eight
  mutation tests successfully. Historical Dice/Words manifests remain pinned.

Built candidate SHA-256 identities:

```text
BIN b5b6034a74bc47fc3eac04d50a0b9d0406aa2b45c89070e904c69de5fd0830e1
ELF 15af9e5b61f85bd11acd8ba3bd8e332aa7a4639b807c29f74041737eb1a3c464
```

These are local build identities, not a published release or installed image.
No new hardware flash, physical acceptance, resource certification, reproducible
build comparison or real-secret readiness is claimed. This milestone is verified
locally; commit, push, merge and hardware installation remain separate actions.
