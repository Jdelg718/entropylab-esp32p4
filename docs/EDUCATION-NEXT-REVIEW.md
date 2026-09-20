# Next education source candidate

Base: `fb600dda52a490f86421f0bfdcc37a4f31ccbed0`. This is a future source-only candidate, not a released or installed firmware claim. Historical input explanations, release pins, binaries and archives remain untouched.

## Findings and implemented walkthrough

1. **BitBox:** `fixture-firmware/input-methods/extra-dice/extra_dice.c`, `bitbox_scan`, consumes five accepted 1-4 values, subtracts one, encodes base 4, then appends a binary bit from a fresh 1-6 outcome. It discards 5/6 only during D4 positions. The lesson now separates physical rejection from the coin phase, supplies an explicit coin-to-key convention, explains why the fair-die rejection preserves equal chances (not how to repair bias), and warns against selective rerolls. PUBLIC keys `123416` produce zero-based index 217 / word number 218. Inserting rejected rolls as `516235416` produces the same index.
2. **BitBox final UI correction:** `fixture-firmware/app/main/extra_dice_native.c`, `refresh` and `final_event`, disable browsing until **Use final #1** is pressed. **Final - / Final +** then immediately change the selected candidate; there is no additional confirmation key. The former instruction to browse then explicitly choose was misleading. Copy now matches the actual sequence. No event handler, result ownership or selection algorithm changed.
3. **D++:** `extra_dice.c`, `dplus_scan`, maps `(D8-1)*256 + hex1*16 + hex2`. A physical D16 labeled 1-16 must be mapped consistently to 0-F; one already labeled 0-F is entered directly. PUBLIC physical faces 3 / 11 / 16 become `3AF`, index 687 / word number 688. Every outcome requires a fresh roll, not reusing a result twice.
4. **D++ final:** `el_dplus_final_index` and native `dplus_final_radix` agree on the displayed sequence for each word count. The 18-word final D8 uses all eight physical faces, reduced into two groups. PUBLIC `A6` selects candidate index 21, not word number 21. Tests exercise `A1` through `A8`. The existing 12/15/18/21/24 final instructions and no-discard warning remain.
5. **Cards:** `fixture-firmware/integrated-lanes/playingcards/target-adapter/cards_target.c`, `direct`, encodes three A-8 ranks and one A-4 rank per prefix word. The lesson now gives a concrete packet recipe: one card per enabled rank, set other cards aside, mix without tracking, draw blind, record exactly, return the physical card, mix afresh and rebuild the packet when the phase changes. Returning a physical card is distinct from substituting a recorded rank. The former phrase “replacing the drawn rank” blurred this distinction. Replacement restores equal composition but does not itself establish independence. PUBLIC `A283` yields bits `000 001 111 10`, index 62 / word number 63. Tests call the actual target converter at all five word counts and reject an 8 in the A-4 position.

All worked examples are deliberately public and predictable, identify whether an index is zero-based or a word number is one-based, and are explicitly forbidden for funding. Single-prefix examples are not represented as complete mnemonics.

## Six-lesson scope and usability boundaries

- Words, Seed and Bases already contain numbered instructions and deterministic-source warnings; this bounded pass leaves their copy unchanged rather than rewriting all six lessons. It does not claim a new complete behavioral audit of those three editors.
- `navigation.inc:active_education_document` selects all six typed documents. `mnemonic_editor.inc:mn_about_present` renders static strings, not input/result/passphrase contents. Its practice text buffer is 1024 bytes; the new regression checks all six practice fields including the public warning and NUL capacity, plus ASCII-only copy.
- `mn_about_present` dynamically positions each section by rendered height and resets the scroll position. Back and Close are outside the scrolling body. No layout, input, navigation, result or core algorithm source was edited.
- Existing `fixture-firmware/tests/education_tests.inc:education_matrix` already covers native top/middle/bottom scrolling, fixed Back/Close geometry, pointer click-through and editor-state preservation. That matrix **was not executed successfully for this candidate**: the isolated worktree lacks LVGL 9.5.0 sources. Longer copy still needs that native matrix and visual review. A buffer/ASCII test is not proof of layout or glyph rendering.

## Verification

Run the bounded regression without Rust, LVGL or historical fixture rebuilding:

```sh
python3 scripts/test-education-examples.py
```

It compiles the unchanged actual extra-dice and Cards target C implementations with `cc -shared -fPIC -std=c11 -Wall -Wextra -Werror`, into a temporary directory, and executes the public examples. It also checks lesson process instructions and all six practice-field limits.

New behavior checks were observed failing before their corresponding copy edits. During development, the actual-core check caught an incorrect draft BitBox arithmetic result; the final documented result is 217 and passes the real parser.

Required broader commands were attempted, not bypassed:

- `timeout 60 bash scripts/test-host.sh`: exit 1 at pre-existing `review-repair current mismatch: fixture-firmware/app/main/gui.c`. That file is unchanged from the base. No historical identity manifests were rewritten to manufacture a pass.
- `EDUCATION_ONLY=1 timeout 60 bash scripts/test-gui-host.sh`: exit 1, `Set LVGL_SOURCE_DIR to LVGL 9.5.0 sources, or build firmware first.` No target build or giant historical host rebuild was attempted.

Remaining gates: supply a reviewed matching LVGL/native harness and run the education pointer/layout/state matrix on these exact sources; separately resolve the predecessor's historical host identity mismatch without modifying accepted pins in this task; review screenshots and target size/build; only with separate authorization perform hardware testing, flash or publication. This candidate makes no hardware acceptance claim.
