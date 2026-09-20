#include "seed_helpers_target.h"

#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "../../../app/main/passphrase_core.h"
#include "../../../app/main/bip39_dictionary.inc"

#define SH_OUTPUT_SIZE 480u

typedef struct {
    char text[216];
    char *tokens[24];
    size_t count;
} parsed_input;

static bool valid_total(uint32_t total) {
    return total == 12 || total == 15 || total == 18 || total == 21 || total == 24;
}

static int dictionary_index(const char *word) {
    int low = 0;
    int high = 2047;
    while (low <= high) {
        const int mid = low + (high - low) / 2;
        const int order = strcmp(word, mn_dictionary[mid]);
        if (order == 0) return mid;
        if (order < 0)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return -1;
}

const char *seed_helpers_word_at(uint32_t zero_based_index) {
    return zero_based_index < 2048u ? mn_dictionary[zero_based_index] : NULL;
}

static int32_t parse_input(const uint8_t *input, size_t input_len,
                           uint32_t operation, size_t expected,
                           parsed_input *parsed) {
    bool previous_space = false;
    size_t count = 1;
    if (input[0] == ' ' || input[input_len - 1] == ' ') return SH_TEXT;
    for (size_t i = 0; i < input_len; ++i) {
        const uint8_t byte = input[i];
        if (byte == ' ') {
            if (previous_space) return SH_TEXT;
            previous_space = true;
            ++count;
        } else {
            const bool valid = operation == SH_NUMBERS_TO_WORDS
                                   ? byte >= '0' && byte <= '9'
                                   : byte >= 'a' && byte <= 'z';
            if (!valid) return SH_TEXT;
            previous_space = false;
        }
    }
    if (count != expected) return SH_COUNT;
    memcpy(parsed->text, input, input_len);
    parsed->text[input_len] = '\0';
    parsed->count = count;
    parsed->tokens[0] = parsed->text;
    size_t token = 1;
    for (size_t i = 0; i < input_len; ++i) {
        if (parsed->text[i] == ' ') {
            parsed->text[i] = '\0';
            parsed->tokens[token++] = &parsed->text[i + 1];
        }
    }
    return SH_OK;
}

static int32_t parse_number(const char *text, uint32_t base, uint16_t *value) {
    if (text[0] == '0' && text[1] != '\0') return SH_NUMBER;
    uint32_t parsed = 0;
    for (const char *cursor = text; *cursor; ++cursor) {
        const uint32_t digit = (uint32_t)(*cursor - '0');
        if (parsed > (UINT32_MAX - digit) / 10u) return SH_NUMBER;
        parsed = parsed * 10u + digit;
    }
    if (parsed < base || parsed > 2047u + base) return SH_NUMBER;
    *value = (uint16_t)parsed;
    return SH_OK;
}

static int32_t final_candidates(const parsed_input *parsed, uint32_t total,
                                sh_output_v1 *scratch) {
    char phrase[216];
    size_t prefix_len = 0;
    for (size_t i = 0; i < parsed->count; ++i) {
        const size_t length = strlen(parsed->tokens[i]);
        if (i) phrase[prefix_len++] = ' ';
        memcpy(&phrase[prefix_len], parsed->tokens[i], length);
        prefix_len += length;
    }
    phrase[prefix_len++] = ' ';

    uint8_t canonical[216];
    uint32_t count = 0;
    for (uint32_t index = 0; index < 2048; ++index) {
        const size_t word_len = strlen(mn_dictionary[index]);
        if (prefix_len + word_len > 215) return SH_BOUNDS;
        memcpy(&phrase[prefix_len], mn_dictionary[index], word_len);
        const int32_t status = el_input_to_mnemonic(
            (const uint8_t *)phrase, prefix_len + word_len, 4, total,
            canonical, sizeof(canonical));
        if (status == 0) {
            if (count >= 128) return SH_BOUNDS;
            scratch->numbers[count++] = (uint16_t)index;
        } else if (status != -6) {
            return status;
        }
    }
    const uint32_t expected = 1u << (11u - total / 3u);
    if (count != expected) return SH_BOUNDS;
    scratch->number_count = count;
    return SH_OK;
}

int32_t seed_helpers_target_v1(const uint8_t *input, size_t input_len,
                               uint32_t operation, uint32_t total,
                               uint32_t base, sh_output_v1 *output,
                               size_t output_bytes) {
    if (!input || !output || input_len == 0 || input_len > 215 ||
        ((uintptr_t)output % _Alignof(sh_output_v1)) != 0 ||
        output_bytes > (size_t)PTRDIFF_MAX) {
        return SH_BOUNDS;
    }
    const uintptr_t input_begin = (uintptr_t)input;
    const uintptr_t output_begin = (uintptr_t)output;
    if (input_begin > UINTPTR_MAX - input_len ||
        output_begin > UINTPTR_MAX - output_bytes) {
        return SH_BOUNDS;
    }
    const uintptr_t input_end = input_begin + input_len;
    const uintptr_t output_end = output_begin + output_bytes;
    if (output_bytes && input_begin < output_end && output_begin < input_end)
        return SH_BOUNDS;
    if (operation < SH_WORDS_TO_NUMBERS || operation > SH_FINAL_WORDS ||
        !valid_total(total) || base > 1 ||
        (operation == SH_FINAL_WORDS && base != 0)) {
        return SH_SELECTOR;
    }

    parsed_input parsed = {0};
    const size_t expected = total - (operation == SH_FINAL_WORDS ? 1u : 0u);
    int32_t status = parse_input(input, input_len, operation, expected, &parsed);
    if (status != SH_OK) return status;

    sh_output_v1 scratch;
    memset(&scratch, 0, sizeof(scratch));
    if (operation == SH_WORDS_TO_NUMBERS || operation == SH_FINAL_WORDS) {
        for (size_t i = 0; i < parsed.count; ++i) {
            const int index = dictionary_index(parsed.tokens[i]);
            if (index < 0) return -100 - (int32_t)i;
            if (operation == SH_WORDS_TO_NUMBERS)
                scratch.numbers[i] = (uint16_t)(index + (int)base);
        }
        if (operation == SH_WORDS_TO_NUMBERS)
            scratch.number_count = total;
        else {
            status = final_candidates(&parsed, total, &scratch);
            if (status != SH_OK) return status;
        }
    } else {
        size_t written = 0;
        for (size_t i = 0; i < parsed.count; ++i) {
            uint16_t value = 0;
            status = parse_number(parsed.tokens[i], base, &value);
            if (status != SH_OK) return status;
            const char *word = mn_dictionary[value - base];
            const size_t word_len = strlen(word);
            if (i) scratch.text[written++] = ' ';
            memcpy(&scratch.text[written], word, word_len);
            written += word_len;
        }
        scratch.text_len = (uint32_t)written;
    }

    if (output_bytes < sizeof(scratch)) return SH_CAPACITY;
    memcpy(output, &scratch, sizeof(scratch));
    return SH_OK;
}

int32_t seed_target_convert(uint32_t operation, uint32_t total, uint32_t base,
                            uint32_t context, const uint8_t *input,
                            size_t input_len, pc_result_v1 *output) {
    if (!output) return SH_BOUNDS;
    if (operation == SH_FINAL_WORDS) return SH_SELECTOR;

    sh_output_v1 helper;
    int32_t status = seed_helpers_target_v1(input, input_len, operation, total,
                                            base, &helper, sizeof(helper));
    if (status != SH_OK) return status;

    const uint8_t *phrase = operation == SH_WORDS_TO_NUMBERS ? input : helper.text;
    const size_t phrase_len = operation == SH_WORDS_TO_NUMBERS
                                  ? input_len
                                  : (size_t)helper.text_len;
    uint8_t canonical[216] = {0};
    status = el_input_to_mnemonic(phrase, phrase_len, 4, total,
                                  canonical, sizeof(canonical));
    if (status != 0) return status;

    const char *method = operation == SH_WORDS_TO_NUMBERS
                             ? "seed-words"
                             : "seed-numbers";
    const size_t method_len = strlen(method);
    pc_result_v1 scratch;
    memset(&scratch, 0, sizeof(scratch));
    scratch.version = PC_V1_VERSION;
    scratch.mode = PC_V1_DIRECT;
    scratch.words = total;
    scratch.context = context;
    scratch.mnemonic_len = (uint32_t)phrase_len;
    scratch.method_len = (uint32_t)method_len;
    memcpy(scratch.mnemonic, canonical, phrase_len);
    memcpy(scratch.method, method, method_len);
    memcpy(output, &scratch, sizeof(scratch));
    return SH_OK;
}

_Static_assert(sizeof(sh_output_v1) == SH_OUTPUT_SIZE, "accepted Seed ABI size");
_Static_assert(offsetof(sh_output_v1, text) == 8, "accepted Seed ABI text offset");
_Static_assert(offsetof(sh_output_v1, numbers) == 224, "accepted Seed ABI numbers offset");
