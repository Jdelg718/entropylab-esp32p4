#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <cstdlib>

extern "C" {
#include "passphrase_core.h"
#include "seed_helpers_target.h"
}
#include "all_features_application.h"

static const char *VALID =
    "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
static const char *INVALID =
    "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon";
static const char *NUMBERS_ZERO = "0 0 0 0 0 0 0 0 0 0 0 3";

static uint64_t queued;
static unsigned locks;
static unsigned conversions;
static unsigned derivations;
static unsigned results;
static unsigned cancellations;
static bool cancel_after_conversion;
static gui08_request published_request;
static pc_result_v1 published_conversion;
static all_features_derivation_result published_derivation;

extern "C" int32_t __real_el_bip39_passphrase_run(
    const uint8_t *, size_t, const uint8_t *, size_t, uint8_t *, size_t,
    uint8_t *, size_t, uint8_t *, size_t);
extern "C" int32_t __wrap_el_bip39_passphrase_run(
    const uint8_t *mnemonic, size_t mnemonic_len, const uint8_t *passphrase,
    size_t passphrase_len, uint8_t *entropy, size_t entropy_cap,
    uint8_t *fingerprint, size_t fingerprint_cap, uint8_t *address,
    size_t address_cap) {
    ++derivations;
    return __real_el_bip39_passphrase_run(
        mnemonic, mnemonic_len, passphrase, passphrase_len, entropy, entropy_cap,
        fingerprint, fingerprint_cap, address, address_cap);
}

static void lock_app() { assert(locks++ == 0); }
static void unlock_app() { assert(locks-- == 1); }
static bool publish(uint64_t token) {
    /* CONTRACT-ERRATUM.md supersedes only queue-publication lock placement. */
    assert(locks == 0 && queued == 0);
    queued = token;
    return true;
}
static gui08_passphrase_marker snapshot(const gui08_request *, uint8_t out[256],
                                        size_t *length) {
    std::memset(out, 0, 256);
    *length = 0;
    return GUI08_PASSPHRASE_EMPTY;
}
static bool pending(const gui08_request *) { return true; }
static bool result(const gui08_request *request, const pc_result_v1 *conversion,
                   const all_features_derivation_result *derivation) {
    published_request = *request;
    published_conversion = *conversion;
    published_derivation = *derivation;
    ++results;
    return true;
}
static bool cancelled(uint64_t) {
    ++cancellations;
    return true;
}
static int convert(const gui08_request *request, pc_result_v1 *output) {
    ++conversions;
    const uint32_t context = (uint32_t)request->request_id ^
                             (uint32_t)(request->request_id >> 32);
    const int status = (int)seed_target_convert(
        request->seed_operation, request->words, request->base, context,
        (const uint8_t *)request->transcript, request->length, output);
    if (cancel_after_conversion)
        assert(all_features_application_cancel(request->request_id));
    return status;
}
static int derive(const gui08_request *request, const pc_result_v1 *conversion,
                  all_features_derivation_result *output) {
    return (int)el_bip39_passphrase_run(
        conversion->mnemonic, conversion->mnemonic_len, request->passphrase,
        request->passphrase_len, (uint8_t *)output->entropy,
        sizeof output->entropy, (uint8_t *)output->fingerprint,
        sizeof output->fingerprint, (uint8_t *)output->address,
        sizeof output->address);
}
static gui08_request seed_request(uint64_t id, unsigned operation,
                                  unsigned base, const char *input) {
    gui08_request request{};
    request.request_id = id;
    request.revision = id;
    request.method = GUI08_SEED;
    request.words = 12;
    request.base = base;
    request.seed_operation = operation;
    request.length = std::strlen(input);
    std::memcpy(request.transcript, input, request.length + 1);
    return request;
}
static void run_queued() {
    assert(queued != 0);
    const uint64_t token = queued;
    queued = 0;
    all_features_application_work(token, convert);
    all_features_application_poll();
}

using seed_oracle_fn = int32_t (*)(const uint8_t *, size_t, uint32_t,
                                   uint32_t, uint32_t, sh_output_v1 *, size_t);

static void helper_contract() {
    const char *oracle_path=std::getenv("SEED_HELPERS_ORACLE_LIBRARY");
    assert(oracle_path && "set SEED_HELPERS_ORACLE_LIBRARY to independently built oracle");
    void *library = dlopen(
        oracle_path,
        RTLD_NOW | RTLD_LOCAL);
    assert(library);
    seed_oracle_fn oracle =
        (seed_oracle_fn)dlsym(library, "el_seed_helpers_v1");
    assert(oracle);

    auto parity = [&](const char *input, uint32_t operation, uint32_t total,
                      uint32_t base, sh_output_v1 *output) {
        sh_output_v1 expected{};
        const size_t length = std::strlen(input);
        const int32_t actual_status = seed_helpers_target_v1(
            (const uint8_t *)input, length, operation, total, base, output,
            sizeof *output);
        const int32_t expected_status = oracle(
            (const uint8_t *)input, length, operation, total, base, &expected,
            sizeof expected);
        assert(actual_status == expected_status);
        assert(actual_status == SH_OK);
        assert(std::memcmp(output, &expected, sizeof expected) == 0);
    };

    sh_output_v1 output{};
    parity(INVALID, SH_WORDS_TO_NUMBERS, 12, 0, &output);
    assert(output.number_count == 12);
    for (unsigned i = 0; i < 12; ++i) assert(output.numbers[i] == 0);

    std::memset(&output, 0, sizeof output);
    parity(NUMBERS_ZERO, SH_NUMBERS_TO_WORDS, 12, 0, &output);
    assert(output.text_len == std::strlen(VALID));
    assert(std::memcmp(output.text, VALID, output.text_len) == 0);

    const char *prefix =
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon";
    std::memset(&output, 0, sizeof output);
    parity(prefix, SH_FINAL_WORDS, 12, 0, &output);
    assert(output.number_count == 128);
    bool found_about = false;
    for (unsigned i = 0; i < output.number_count; ++i) {
        if (i) assert(output.numbers[i - 1] < output.numbers[i]);
        if (output.numbers[i] == 3) found_about = true;
    }
    assert(found_about);
    dlclose(library);
    std::puts("PASS accepted Seed ABI oracle parity + explicit 128-candidate final-word helper");
}

int main() {
    helper_contract();
    assert(all_features_application_configure(
        lock_app, unlock_app, publish, snapshot, derive, pending, result,
        cancelled));

    gui08_request valid = seed_request(2801, SH_WORDS_TO_NUMBERS, 0, VALID);
    const unsigned before_valid = derivations;
    assert(all_features_application_submit(&valid));
    std::memset(valid.transcript, 'x', valid.length);
    run_queued();
    assert(derivations == before_valid + 1 && results == 1);
    assert(published_request.method == GUI08_SEED);
    assert(published_request.seed_operation == SH_WORDS_TO_NUMBERS);
    assert(std::memcmp(published_conversion.mnemonic, VALID,
                       published_conversion.mnemonic_len) == 0);
    assert(std::strcmp(published_derivation.fingerprint, "73c5da0a") == 0);
    std::puts("PASS valid public fixture exactly one derivation + copied request input");

    sh_output_v1 mapping{};
    assert(seed_helpers_target_v1((const uint8_t *)INVALID, std::strlen(INVALID),
                                  SH_WORDS_TO_NUMBERS, 12, 0, &mapping,
                                  sizeof mapping) == SH_OK);
    const unsigned before_invalid = derivations;
    gui08_request invalid = seed_request(2802, SH_WORDS_TO_NUMBERS, 0, INVALID);
    assert(all_features_application_submit(&invalid));
    run_queued();
    assert(derivations == before_invalid && results == 1);
    std::puts("PASS mapping success != seed validity; invalid whole phrase derivations=0");

    const unsigned before_numbers = derivations;
    gui08_request numbers = seed_request(2803, SH_NUMBERS_TO_WORDS, 0, NUMBERS_ZERO);
    assert(all_features_application_submit(&numbers));
    run_queued();
    assert(derivations == before_numbers + 1 && results == 2);
    assert(std::memcmp(published_conversion.mnemonic, VALID,
                       published_conversion.mnemonic_len) == 0);
    std::puts("PASS canonical numeric mapping uses same exactly-once derivation lifecycle");

    const unsigned before_cancel = derivations;
    gui08_request stale = seed_request(2804, SH_WORDS_TO_NUMBERS, 0, VALID);
    assert(all_features_application_submit(&stale));
    cancel_after_conversion = true;
    run_queued();
    cancel_after_conversion = false;
    assert(derivations == before_cancel && results == 2);

    gui08_request queued_cancel = seed_request(2805, SH_WORDS_TO_NUMBERS, 0, VALID);
    assert(all_features_application_submit(&queued_cancel));
    const uint64_t stale_token = queued;
    assert(all_features_application_cancel(queued_cancel.request_id));
    queued = 0;
    all_features_application_work(stale_token, convert);
    all_features_application_poll();
    assert(derivations == before_cancel && results == 2);
    std::puts("PASS running cancellation + stale queued token suppress derivation/publication");

    all_features_application_dispose();
    std::printf("PASS Seed target lifecycle conversions=%u derivations=%u results=%u cancellations=%u\n",
                conversions, derivations, results, cancellations);
}
