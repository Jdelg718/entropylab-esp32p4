#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "all_features_application.h"
#include "lifehash_fingerprint.h"

static unsigned lock_depth;
static uint64_t queued_token;
static bool queue_accept = true;
static unsigned kdf_calls;
static unsigned render_calls;
static unsigned image_calls;
static unsigned blank_calls;
static unsigned result_calls;
static unsigned cancelled_calls;
static bool cancel_in_render;
static bool malformed_fingerprint;
static bool tamper_epoch;
static bool textual_result_accepted;
static uint8_t published_rgb[ALL_FEATURES_LIFEHASH_RGB_SIZE];

static void lock_app() { assert(lock_depth++ == 0); }
static void unlock_app() { assert(lock_depth-- == 1); }
static bool publish(uint64_t token) {
    assert(lock_depth == 0); /* Erratum: queue publication is outside transition lock. */
    if (!queue_accept || queued_token) return false;
    queued_token = token;
    return true;
}
static gui08_passphrase_marker snapshot(const gui08_request *, uint8_t out[256], size_t *length) {
    std::memset(out, 0, 256); *length = 0; return GUI08_PASSPHRASE_EMPTY;
}
static int convert(const gui08_request *, pc_result_v1 *out) {
    std::memset(out, 0, sizeof *out);
    out->version = PC_V1_VERSION; out->mode = PC_V1_DIRECT; out->words = 12;
    out->mnemonic_len = 1; out->mnemonic[0] = 'x';
    return 0;
}
static int derive(const gui08_request *, const pc_result_v1 *, all_features_derivation_result *out) {
    assert(lock_depth == 0); ++kdf_calls; std::memset(out, 0, sizeof *out);
    std::strcpy(out->entropy, "00");
    std::strcpy(out->fingerprint, malformed_fingerprint ? "73c5da0g" : "73c5da0a");
    std::strcpy(out->address, "bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"); return 0;
}
static int render(const all_features_lifehash_request *request,
                  all_features_lifehash_result *result) {
    assert(lock_depth == 0); ++render_calls;
    const uint8_t expected[4] = {0x73, 0xc5, 0xda, 0x0a};
    assert(request && result && std::memcmp(request->raw4, expected, 4) == 0);
    if (cancel_in_render) assert(all_features_application_cancel(3702));
    const int status = (int)lifehash_fingerprint_render(
        request->raw4, 4, LIFEHASH_FINGERPRINT_VERSION_2,
        result->rgb, sizeof result->rgb);
    if (tamper_epoch) ++result->route_epoch;
    return status;
}
static bool pending(const gui08_request *) { return true; }
static bool result(const gui08_request *, const pc_result_v1 *, const all_features_derivation_result *) {
    textual_result_accepted = true; ++result_calls; return true;
}
static bool image(const gui08_request *, const char fingerprint[9], const uint8_t *rgb, size_t length) {
    assert(textual_result_accepted); assert(std::strcmp(fingerprint, "73c5da0a") == 0);
    assert(length == sizeof published_rgb); std::memcpy(published_rgb, rgb, length); ++image_calls; return true;
}
static void blank() { ++blank_calls; std::memset(published_rgb, 0, sizeof published_rgb); }
static bool cancelled(uint64_t) { ++cancelled_calls; return true; }

static gui08_request request(uint64_t id) {
    gui08_request value{}; value.request_id = id; value.revision = id;
    value.method = GUI08_CARDS; value.words = 12; value.length = 47;
    std::memset(value.transcript, 'A', value.length); value.transcript[value.length] = 0;
    return value;
}
static void work_poll() {
    assert(queued_token); const uint64_t token = queued_token; queued_token = 0;
    all_features_application_work(token, convert); all_features_application_poll();
}

int main() {
    assert(all_features_application_configure(lock_app, unlock_app, publish, snapshot,
                                               derive, pending, result, cancelled));
    assert(all_features_application_lifehash_configure(render, image, blank));

    queue_accept = false; gui08_request rejected = request(3700);
    assert(!all_features_application_submit(&rejected));
    assert(kdf_calls == 0 && render_calls == 0 && image_calls == 0);

    queue_accept = true; gui08_request first = request(3701);
    assert(all_features_application_submit(&first));
    std::memset(first.transcript, 'x', first.length); /* owned request */
    work_poll();
    assert(kdf_calls == 1 && render_calls == 1 && result_calls == 1 && image_calls == 1);
    all_features_application_poll();
    assert(image_calls == 1); /* duplicate take rejected */

    gui08_request stale = request(3702); assert(all_features_application_submit(&stale));
    cancel_in_render = true; work_poll(); cancel_in_render = false;
    assert(kdf_calls == 2 && render_calls == 2 && image_calls == 1);
    assert(blank_calls >= 3); /* submit invalidation, cancel, stale suppression */

    gui08_request before_claim = request(3703); assert(all_features_application_submit(&before_claim));
    const uint64_t stale_token = queued_token; queued_token = 0;
    assert(all_features_application_cancel(before_claim.request_id));
    all_features_application_work(stale_token, convert); all_features_application_poll();
    assert(kdf_calls == 2 && render_calls == 2 && image_calls == 1);

    /* Dangerous malformed fingerprint is rejected before rendering or image
     * publication; a following benign request proves recovery is transactional. */
    malformed_fingerprint = true;
    gui08_request malformed = request(3704); assert(all_features_application_submit(&malformed));
    work_poll(); malformed_fingerprint = false;
    assert(kdf_calls == 3 && render_calls == 2 && result_calls == 1 &&
           image_calls == 1 && cancelled_calls == 1);
    gui08_request after_malformed = request(3705);
    assert(all_features_application_submit(&after_malformed)); work_poll();
    assert(kdf_calls == 4 && render_calls == 3 && result_calls == 2 &&
           image_calls == 2 && cancelled_calls == 1);

    /* Dangerous renderer identity tampering cannot publish across the epoch
     * boundary; the next benign matching identity still publishes normally. */
    tamper_epoch = true;
    gui08_request tampered = request(3706); assert(all_features_application_submit(&tampered));
    work_poll(); tamper_epoch = false;
    assert(kdf_calls == 5 && render_calls == 4 && result_calls == 2 &&
           image_calls == 2 && cancelled_calls == 2);
    gui08_request after_tamper = request(3707);
    assert(all_features_application_submit(&after_tamper)); work_poll();
    assert(kdf_calls == 6 && render_calls == 5 && result_calls == 3 &&
           image_calls == 3 && cancelled_calls == 2);

    all_features_application_dispose();
    assert(blank_calls >= 5);
    std::puts("PASS LifeHash application lifecycle: queue rollback, raw4 copy, off-lock KDF/render, cancellation/stale/duplicate suppression, malformed fingerprint rejection+recovery, tampered epoch rejection+recovery, blank invalidation, no extra KDF");
}
