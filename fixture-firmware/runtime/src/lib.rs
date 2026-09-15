#![cfg_attr(target_os = "espidf", no_std)]
// Source-level rlib composition. Public reexports retain the C API dependencies.
pub use entropylab_hex_core::el_hex_run;
pub use entropylab_dice_core::{el_dice_to_hex, el_dice_required_rolls};
pub use entropylab_mnemonic_core::el_mnemonic_run;
pub use entropylab_coin_core::el_coin_to_hex;
