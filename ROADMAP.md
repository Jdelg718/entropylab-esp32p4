# Roadmap and acceptance gates

1. **Public fixture (current):** compiled BIP39/BIP32/BIP84 computation, LVGL touch
   trigger, bounded C ABI and recorded hardware PASS. Reproduce target build from
   this standalone layout before describing it as clean-checkout target-verified.
2. **GUI and test-input calculator:** integrate the reviewed portrait design in
   native LVGL; bounded public entropy and ASCII passphrase input; explicit path
   limits; negative tests; repeated device runs; no signing. Browser prototype
   work remains separate until deliberately reviewed and integrated.
3. **Reliability:** queue saturation, startup failure injection, allocation failure,
   cancellation/reset semantics, soak tests, stack/RAM headroom and dependency review.
4. **Broader calculator features:** each feature requires independent published
   vectors, architecture review, input bounds and hardware tests.

Real-secret handling, signing, storage, RNG and networking are not approved scope.
They require a new threat model and explicit decision, not incremental UI additions.
Public release requires independent content/privacy/license audit and explicit
publication approval. Contributions are via reviewed changes, not automatic access.
