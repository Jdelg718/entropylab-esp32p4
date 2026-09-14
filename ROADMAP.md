# Roadmap and acceptance gates

1. **Public fixture (historical):** compiled BIP39/BIP32/BIP84 computation, LVGL touch
   trigger, bounded C ABI and recorded hardware PASS. Reproduce target build from
   this standalone layout before describing it as clean-checkout target-verified.
2. **Public-test HEX calculator (current local milestone):** native LVGL HEX
   input, 12–24 BIP39 words, empty passphrase only, first mainnet BIP84 address;
   bounded ABI, real-core GUI host tests and source hardware observations.
   See docs/HEX-MILESTONE.md. Independent repository review precedes publication.
   Passphrase input and broader paths remain unimplemented future scope.
3. **Reliability:** queue saturation, startup failure injection, allocation failure,
   cancellation/reset semantics, soak tests, stack/RAM headroom and dependency review.
4. **Broader calculator features:** each feature requires independent published
   vectors, architecture review, input bounds and hardware tests.

Real-secret handling, signing, storage, RNG and networking are not approved scope.
They require a new threat model and explicit decision, not incremental UI additions.
Public release requires independent content/privacy/license audit and explicit
publication approval. Contributions are via reviewed changes, not automatic access.
