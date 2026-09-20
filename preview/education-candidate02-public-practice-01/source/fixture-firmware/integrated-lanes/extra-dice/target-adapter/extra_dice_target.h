#ifndef ENTROPYLAB_EXTRA_DICE_TARGET_H
#define ENTROPYLAB_EXTRA_DICE_TARGET_H

#include "gui08_editor.h"
#include "../../playingcards/adapter/include/playing_cards_host_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Converts an owned GUI08_DICE request into one checksum-valid canonical
 * mnemonic. Mapping success alone is not acceptance: the explicit final choice
 * is resolved against the checksum-valid candidate list before publication. */
int extra_dice_target_convert(const gui08_request *request, uint32_t context,
                              pc_result_v1 *output);

#ifdef __cplusplus
}
#endif
#endif
