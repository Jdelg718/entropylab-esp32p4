/* Static public education. Never interpolate editor, result, or passphrase data. */
typedef struct {
 const char *title;
 const char *before_practice;
 const char *practice;
 const char *after_practice;
} education_document;

static const education_document education_words={
 "Words",
 "#ff9900 What it does#\nWords accepts typed BIP39 English words and validates the selected count, dictionary membership, and checksum.\n\n"
 "#ff9900 When to choose it#\nUse it to import or review a phrase produced elsewhere. This route does not generate randomness.\n\n"
 "#ff9900 What you need#\nA public practice phrase or an existing phrase from an external process. Never enter a funded phrase in this laboratory build.\n\n"
 "#ff9900 Steps#\n1. Choose 12, 15, 18, 21, or 24 words.\n2. Type a prefix with the letter keys.\n3. Select a displayed dictionary candidate.\n4. Review committed words; replace a word if needed.\n5. Tap Validate when the selected count is complete.\n\n"
 "#ff9900 Where unpredictability comes from#\nOnly from the external process that produced the phrase. Typing and dictionary lookup add none.\n\n"
 "#ff9900 What adds no entropy#\nDictionary checks, checksum validation, hashing, PBKDF2, and LifeHash are deterministic and add no entropy.\n\n"
 "#ff9900 Accepted input, discards, and limits#\nOnly displayed BIP39 English dictionary candidates can be committed. Supported counts are 12, 15, 18, 21, and 24. Words accepts words; Seed accepts word-list numbers and has number preview and a final-candidate helper. Both import existing entropy; neither invents it.",
 "Use a conspicuously known phrase only to learn the editor, for example eleven copies of 'abandon' followed by 'about' at 12 words. It is predictable and must never receive funds.",
 "#ff9900 Pitfalls#\nA mistyped but valid word may survive validation. A valid checksum is not proof of correctness, ownership, fairness, or entropy. Chosen or repeated phrases are predictable.\n\n"
 "#ff9900 Deeper detail#\nRecovered entropy is data encoded by the words, not a measurement of randomness quality.\n\n"
 "#ff9900 No strength promise#\nThis screen validates structure only. It cannot certify that the external source was unpredictable or safe."
};

static const education_document education_seed={
 "Seed - Word numbers",
 "#ff9900 What it does#\nSeed accepts word-list numbers, previews their BIP39 English words, and helps restrict the final word to checksum-valid candidates.\n\n"
 "#ff9900 When to choose it#\nUse it to import numbers produced elsewhere. It does not generate randomness.\n\n"
 "#ff9900 What you need#\nA public practice list or existing BIP39 word indices, plus knowledge of whether the list uses 1 through 2048 or 0 through 2047.\n\n"
 "#ff9900 Steps#\n1. Choose the word count.\n2. Set the displayed number origin: 1 through 2048 or 0 through 2047.\n3. Enter up to four digits and check the word preview.\n4. Tap Next word to commit each prefix number.\n5. For the last position, choose or enter a displayed checksum-valid candidate.\n6. Continue only when all positions are complete.\n\n"
 "#ff9900 Where unpredictability comes from#\nOnly from the external process that produced the numbers. Selecting the final candidate contributes entropy only when that choice itself comes from an independent unpredictable process.\n\n"
 "#ff9900 What adds no entropy#\nNumber-to-word mapping, preview, checksum calculation, hashing, and LifeHash are deterministic and add no entropy.\n\n"
 "#ff9900 Accepted input, discards, and limits#\nThe keypad accepts digits for one number at a time. Use 1 through 2048 in 1-based mode or 0 through 2047 in 0-based mode. The final number must be in the displayed valid set. Seed accepts numbers; Words accepts typed dictionary words.",
 "At 12 words, enter 1 eleven times in 1 through 2048 mode, then inspect the displayed final candidates. This known all-zero prefix is predictable and must never receive funds.",
 "#ff9900 Pitfalls#\nChanging the number origin changes interpretation. A checksum-valid final number does not prove earlier entries are correct or random. Patterns and remembered choices are predictable.\n\n"
 "#ff9900 Deeper detail#\nThe two origins are display conventions for the same 2048-entry word list.\n\n"
 "#ff9900 No strength promise#\nThis helper checks range and structure only. It cannot certify the source of the imported numbers."
};

static const education_document education_cards={
 "Cards",
 "#ff9900 What it does#\nCards encodes direct rank phases A-8, A-4, and A-2 into BIP39 input. Suits are not entered, and this firmware has no deck-permutation mode.\n\n"
 "#ff9900 When to choose it#\nUse it when an external card-drawing process supplies the rank requested on screen.\n\n"
 "#ff9900 What you need#\nUse one card of each enabled rank, with indistinguishable backs: A-8 for an A-8 phase, A-4 for A-4, and A-2 for A-2. Set all other cards aside. Equal rank counts avoid weighting one rank more than another. The device cannot test fairness.\n\n"
 "#ff9900 Steps#\n1. Choose the word count.\n2. Read the current prompt: A-8, A-4, or A-2.\n3. Mix the matching packet face down, thoroughly and without tracking a card; draw one without looking.\n4. Tap the drawn rank exactly once. Return the drawn card before every new draw, then mix again. Repeated ranks are allowed.\n5. Check the next prompt. Rebuild the packet when its range changes, keeping one card per enabled rank. Never substitute a preferred rank.\n6. Repeat until ready, then continue. The device cannot test fairness.\n\n"
 "#ff9900 Where unpredictability comes from#\nIt comes from independent, unpredictable shuffling and drawing before each recorded rank. Replacement restores equal rank chances; fresh thorough mixing is still necessary. Replacement alone does not make draws independent.\n\n"
 "#ff9900 What adds no entropy#\nRank encoding, disabled-key checks, checksum processing, hashing, and LifeHash are deterministic and add no entropy.\n\n"
 "#ff9900 Accepted input, discards, and limits#\nOnly ranks enabled for the current phase are accepted. Prefix words use A-8, A-8, A-8, then A-4. Final phases vary by word count and are shown by the firmware. Suits are ignored because there is no suit control.",
 "PUBLIC mixed example: A,2,8,3 in the four prefix phases means values 0,1,7,2. The bits are 000 001 111 10, giving zero-based index 62 (word number 63). Tap ranks, not these index digits. This is one prefix word, not a complete phrase; follow the changing final prompts to finish. Repeatedly tapping A is another predictable flow demonstration. These examples must never receive funds.",
 "#ff9900 Pitfalls#\nPoor shuffling, drawing without replacement, marked backs, unequal rank counts, or substituting a chosen rank can bias or correlate input. Returning the physical card is required by this recipe; changing the recorded rank is not. The device accepts valid ranks but cannot verify the physical draw.\n\n"
 "#ff9900 Deeper detail#\nDirect rank encoding is the only Cards algorithm implemented here. Do not apply full-deck or permutation instructions to this screen.\n\n"
 "#ff9900 No strength promise#\nA completed count or valid result cannot certify shuffle quality, independence, or security."
};

static const education_document education_bases={
 "Number bases",
 "#ff9900 What it does#\nNumber bases encodes externally produced bits with base 4, 8, 32, or 64 symbols. Encoding is not an entropy source.\n\n"
 "#ff9900 When to choose it#\nUse it when you already have external entropy represented in one of the displayed alphabets.\n\n"
 "#ff9900 What you need#\nAn external bit source and its exact symbol transcription. Know the chosen base before entry.\n\n"
 "#ff9900 Steps#\n1. Choose word count and base.\n2. Press only the symbols displayed on the current alphabet page.\n3. Use page controls for base 32 or 64.\n4. At a restricted final position, use only enabled symbols.\n5. For a base-64 remainder phase, enter the enabled 0 or 1 bit keys.\n6. Continue when the target is complete.\n\n"
 "#ff9900 Where unpredictability comes from#\nOnly from the external process that produced the represented bits.\n\n"
 "#ff9900 What adds no entropy#\nBase conversion, pagination, range checks, checksum processing, hashing, and LifeHash are deterministic and add no entropy.\n\n"
 "#ff9900 Accepted input, discards, and limits#\nBase 4 displays 0 through 3; base 8 displays 0 through 7; base 32 displays qpzry9x8gf2tvdw0s3jn54khce6mua7l; base 64 displays A through Z, a through z, 0 through 9, +, and /. Press displayed symbols exactly. Invalid or out-of-range final symbols are disabled or rejected.",
 "Enter a displayed all-zero pattern only to observe target and remainder behavior. It is predictable and must never receive funds.",
 "#ff9900 Pitfalls#\nDo not mistake encoded width for measured entropy. Wrong base, wrong case in base 64, transcription errors, and chosen patterns can produce valid but predictable input.\n\n"
 "#ff9900 Deeper detail#\nSome word-count/base pairs end with a reduced-range symbol; base 64 instead exposes remaining individual bits. Follow the enabled controls.\n\n"
 "#ff9900 No strength promise#\nA full target, checksum, hash width, or LifeHash cannot certify the external source."
};

static const education_document education_bitbox={
 "BitBox Dice",
 "#ff9900 What it does#\nBitBox Dice maps Five D4 outcomes and one coin outcome to each prefix word, then requires an explicit checksum-candidate choice.\n\n"
 "#ff9900 When to choose it#\nUse it with independent physical D6 rolls and a coin-like outcome when you want the implemented BitBox-style direct mapping.\n\n"
 "#ff9900 What you need#\nFive D4 outcomes made with D6 rejection: reroll physical 5 or 6 until each D4 outcome is 1 through 4. Then use an independent coin-like sixth outcome.\n\n"
 "#ff9900 Steps#\n1. Choose the word count.\n2. For each prefix word, enter five accepted D4 values 1 through 4.\n3. If a D6 shows 5 or 6 during a D4 position, reroll; tapping 5 or 6 is accepted but discarded from that position.\n4. Roll the D6 again for the coin position: enter its actual face, with 1-3 giving bit 0 and 4-6 giving bit 1. No face is discarded here. Or flip a coin with a fixed rule: heads -> key 1; tails -> key 4.\n5. Repeat with fresh rolls for each prefix word. Never reroll an accepted face just because you dislike it.\n6. First tap Use final #1. Final - / Final + change the selected candidate immediately; there is no separate confirmation key.\n7. Check the selected candidate, then Continue to Passphrase.\n\n"
 "#ff9900 Where unpredictability comes from#\nIt comes from independent fair physical outcomes. Among accepted D6 rolls, 1-4 have equal chances if the D6 is fair; rejecting 5-6 does not fix a biased die. The fresh coin-position roll splits six faces into two equal groups. Final-candidate selection adds unpredictability only if that choice comes from an independent unpredictable process.\n\n"
 "#ff9900 What adds no entropy#\nDiscard handling, direct mapping, candidate calculation, checksum, hashing, and LifeHash are deterministic and add no entropy.\n\n"
 "#ff9900 Accepted input, discards, and limits#\nThe visible keypad accepts 1 through 6. Values 1 through 4 fill D4 positions; 5 and 6 are discarded there. The sixth accepted group input supplies the coin side. Separators are internal transcript formatting, not keypad controls.",
 "PUBLIC mixed example: 1,2,3,4,1 then 6 gives zero-based index 217 (word number 218). Subtract 1 from each D4: 0,1,2,3,0 form base-4 value 108; multiply by 2 and add coin bit 1. Commas explain order, not keys. This is one prefix word, not a complete phrase. Load public zero separately fills a known all-1 prefix for practicing final selection. These examples are predictable and must never receive funds.",
 "#ff9900 Pitfalls#\nBiased rolls, weak motion, transcription mistakes, selective rerolls, or always choosing the same final candidate reduce unpredictability. The device cannot test fairness.\n\n"
 "#ff9900 Deeper detail#\nFive base-4 positions and one binary outcome select a prefix word directly; the checksum restricts the final candidates.\n\n"
 "#ff9900 No strength promise#\nA complete prefix, valid checksum, or LifeHash cannot certify fair dice or an unpredictable final choice."
};

static const education_document education_dplus={
 "D++ D8/D16",
 "#ff9900 What it does#\nD++ directly maps one D8 position and two D16 hexadecimal positions to each prefix word. This firmware is the authority for this screen; no independent public D++ specification was established.\n\n"
 "#ff9900 When to choose it#\nUse it only when your physical process can supply independent D8 and D16 outcomes matching the displayed controls.\n\n"
 "#ff9900 What you need#\nA fair D8 and D16, rolled afresh for every position. Before rolling, fix this D16 rule: faces 1-16 -> keys 0-9,A-F (subtract 1, then write hex). Thus face 1 -> 0, face 10 -> 9, face 11 -> A, face 16 -> F. For a die already labeled 0-F, enter the label directly. Do not mix these conventions. The device cannot verify physical fairness or your mapping.\n\n"
 "#ff9900 Steps#\n1. Choose the word count.\n2. For each prefix word, enter D8 with keys 1-8, then two D16 outcomes with keys 0 through F.\n3. Repeat until the prefix is complete.\n4. Enter the final positions shown by this firmware: 12 words D8 then D16; 15 words D8 then D8; 18 words: D16 then D8; 21 words D16; 24 words D8.\n5. For 18 words, enter the actual D16 face, then the actual D8 face. Apply the fixed hex mapping to the D16; do not subtract 1 from the D8 key. The D8 is interpreted as one binary bit: D8 faces 1 through 4 become bit 0; faces 5 through 8 become bit 1.\n6. Continue to Passphrase when ready.\n\n"
 "#ff9900 Where unpredictability comes from#\nIt comes only from independent fair physical outcomes and a consistent unbiased mapping to the visible keys. Each D16 face must map to exactly one distinct key. Relabeling preserves fair outcomes; it creates no randomness. For the 18-word final D8, each bit represents four faces, so the split is balanced only if the physical outcomes are fair.\n\n"
 "#ff9900 What adds no entropy#\nDirect index mapping, grouping, final-position constraints, checksum processing, hashing, and LifeHash are deterministic and add no entropy.\n\n"
 "#ff9900 Accepted input, discards, and limits#\nPrefix position one accepts 1 through 8; positions two and three accept 0 through F. Final controls follow the word-count-specific sequence above. For the 18-word final D8, enter its face exactly; the firmware reduces 1-4 to bit 0 and 5-8 to bit 1. The final D8 has no discarded faces. No separator key is available; grouping is handled internally. Every accepted roll position contributes; there is no BitBox-style discard rule.",
 "PUBLIC mixed example: D8 face 3, D16 faces 11 and 16 become keys 3,A,F. 3AF gives zero-based index 687 (word number 688): 2*256 + 10*16 + 15. This is one prefix word, not a full phrase. 18-word final example: D16 face 11 -> A, D8 face 6 -> 6 gives candidate index 21 (zero-based), not word number 21. Load public zero separately fills known 100 prefix groups. These examples are predictable and must never receive funds.",
 "#ff9900 Pitfalls#\nBiased dice, inconsistent D16 mapping, transcription errors, and confusing D++ with BitBox or legacy hashed D6 modes can invalidate the physical procedure.\n\n"
 "#ff9900 Deeper detail#\nThe prefix mapping is (D8 minus 1) times 256 plus two hexadecimal positions. This description does not claim a third-party D++ standard.\n\n"
 "#ff9900 No strength promise#\nA complete sequence, checksum, or displayed result cannot certify dice fairness, independence, or security."
};

static const education_document *active_education_document(void);
static void mn_about_present(void);
