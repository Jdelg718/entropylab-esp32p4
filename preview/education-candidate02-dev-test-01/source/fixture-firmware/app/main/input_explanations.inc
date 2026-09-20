/* Static explanatory copy only. No input, result, or derived state captured. */
static const char *const mn_explanation_names[GUI_MODE_COUNT]={"HEX","Coins","D6 raw","D6 6->0 (Coleman)","Words"};
static const char *const mn_explanation_copy[GUI_MODE_COUNT]={
 "Each hex character encodes 4 bits. Accepted lengths are 32, 40, 48, 56 or 64 characters.\n\n"
 "The displayed bit count is encoded width, not measured randomness. A full-length input can still be predictable.\n\n"
 "This mode uses the entered hex bytes as the BIP39 input. It does not measure how that input was generated.",
 "Heads = 0. Tails = 1. Bits are packed in entry order, most-significant bit first.\n\n"
 "Use exactly 128, 160, 192, 224 or 256 flips for 12, 15, 18, 21 or 24 words, respectively. There is no input hash, padding or truncation.\n\n"
 "The preview shows the latest 64 bits; the full input is retained. Encoded width is not measured randomness. An exact count does not prove fair, independent flips.",
 "Each roll is recorded as its ASCII digit, 1 through 6. SHA-256 hashes ALL entered rolls in order, not just the preview.\n\n"
 "For 12, 15, 18, 21 or 24 words, the first 16, 20, 24, 28 or 32 bytes of the hash are used, respectively.\n\n"
 "Nominal roll counts assume fair, independent dice. WEAK_INPUT_LAB_ONLY means the input is below the nominal count; meeting that count does not prove fairness or randomness quality.\n\n"
 "Hashing cannot manufacture entropy. A predictable roll sequence remains predictable after hashing. Hash output width is not measured randomness.",
 "Each roll is recorded as a digit. Every 6 is mapped to ASCII 0 before hashing; digits 1 through 5 are unchanged. SHA-256 hashes ALL mapped rolls in order, not just the preview.\n\n"
 "For 12, 15, 18, 21 or 24 words, the first 16, 20, 24, 28 or 32 bytes of the hash are used, respectively.\n\n"
 "Nominal roll counts assume fair, independent dice. WEAK_INPUT_LAB_ONLY means the input is below the nominal count; meeting that count does not prove fairness or randomness quality.\n\n"
 "Hashing cannot manufacture entropy. A predictable roll sequence remains predictable after hashing. Hash output width is not measured randomness.",
 "Validation checks the English BIP39 dictionary, the selected word count and the checksum. Supported counts are 12, 15, 18, 21 or 24 words.\n\n"
 "A valid checksum does not establish entropy quality or prove ownership of a wallet. Dictionary membership and word count do not show how the words were generated.\n\n"
 "Recovered entropy is the data encoded by the words, not a measurement of randomness or proof that the input is safe to use."
};
