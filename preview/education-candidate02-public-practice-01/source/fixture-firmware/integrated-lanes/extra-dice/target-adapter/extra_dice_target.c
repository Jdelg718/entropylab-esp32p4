#include "extra_dice_target.h"

#include <string.h>

#include "extra_dice.h"
#include "seed_helpers_target.h"

static int valid_words(unsigned words) {
    return words == 12 || words == 15 || words == 18 ||
           words == 21 || words == 24;
}

static int append(char *text, size_t capacity, size_t *length,
                  const char *value) {
    const size_t value_length = strlen(value);
    if (*length + value_length >= capacity) return 0;
    memcpy(text + *length, value, value_length);
    *length += value_length;
    text[*length] = 0;
    return 1;
}

int extra_dice_target_convert(const gui08_request *request, uint32_t context,
                              pc_result_v1 *output) {
    if (!request || !output || request->method != GUI08_DICE ||
        !valid_words(request->words) || !request->length ||
        request->length > 1024 || request->transcript[request->length] != 0 ||
        request->dice_final_length >= sizeof request->dice_final ||
        request->dice_final[request->dice_final_length] != 0)
        return EL_DICE_INVALID;

    uint16_t indices[23] = {0};
    size_t count = 0;
    int status;
    if (request->dice_method == GUI08_DICE_BITBOX) {
        status = el_bitbox_indices(request->transcript, request->length,
                                   indices, request->words - 1u, &count);
    } else if (request->dice_method == GUI08_DICE_DPLUS) {
        status = el_dplus_indices(request->transcript, request->length,
                                  indices, request->words - 1u, &count);
    } else {
        return EL_DICE_INVALID;
    }
    if (status != EL_DICE_OK || count != request->words - 1u)
        return status == EL_DICE_OK ? EL_DICE_INCOMPLETE : status;

    char prefix[216] = {0};
    size_t prefix_length = 0;
    for (size_t i = 0; i < count; ++i) {
        const char *word = seed_helpers_word_at(indices[i]);
        if (!word || (i && !append(prefix, sizeof prefix, &prefix_length, " ")) ||
            !append(prefix, sizeof prefix, &prefix_length, word))
            return EL_DICE_CAPACITY;
    }

    sh_output_v1 candidates;
    memset(&candidates, 0, sizeof candidates);
    status = (int)seed_helpers_target_v1(
        (const uint8_t *)prefix, prefix_length, SH_FINAL_WORDS,
        request->words, 0, &candidates, sizeof candidates);
    if (status != SH_OK || !candidates.number_count) return status;

    unsigned choice = request->dice_final_choice;
    if (request->dice_method == GUI08_DICE_DPLUS) {
        uint8_t parsed = 0;
        status = el_dplus_final_index(request->dice_final,
                                      request->dice_final_length,
                                      request->words, &parsed);
        if (status != EL_DICE_OK) return status;
        choice = parsed;
    } else if (request->dice_final_length != 0) {
        return EL_DICE_INVALID;
    }
    if (choice >= candidates.number_count) return EL_DICE_INVALID;

    const char *final_word = seed_helpers_word_at(candidates.numbers[choice]);
    if (!final_word || !append(prefix, sizeof prefix, &prefix_length, " ") ||
        !append(prefix, sizeof prefix, &prefix_length, final_word))
        return EL_DICE_CAPACITY;

    status = (int)seed_target_convert(SH_WORDS_TO_NUMBERS, request->words, 0,
                                      context, (const uint8_t *)prefix,
                                      prefix_length, output);
    if (status != SH_OK) return status;
    const char *method = request->dice_method == GUI08_DICE_BITBOX
                             ? "dice-bitbox" : "dice-dplus";
    const size_t method_length = strlen(method);
    memset(output->method, 0, sizeof output->method);
    memcpy(output->method, method, method_length);
    output->method_len = (uint32_t)method_length;
    return SH_OK;
}
