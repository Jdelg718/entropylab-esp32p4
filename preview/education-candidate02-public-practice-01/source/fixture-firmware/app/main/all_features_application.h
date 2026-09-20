#ifndef ENTROPYLAB_ALL_FEATURES_APPLICATION_H
#define ENTROPYLAB_ALL_FEATURES_APPLICATION_H

#include "all_features_dispatch.h"
#include "../../integrated-lanes/playingcards/adapter/include/playing_cards_host_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*all_features_lock_fn)(void);
typedef bool (*all_features_queue_publish_fn)(uint64_t token);
typedef gui08_passphrase_marker (*all_features_passphrase_snapshot_fn)(
    const gui08_request *request, uint8_t passphrase[256], size_t *passphrase_len);
typedef int (*all_features_convert_fn)(const gui08_request *request,
                                       pc_result_v1 *result);
typedef struct {
    char entropy[65];
    char fingerprint[9];
    char address[43];
} all_features_derivation_result;
typedef int (*all_features_derive_fn)(const gui08_request *request,
                                      const pc_result_v1 *conversion,
                                      all_features_derivation_result *result);
typedef bool (*all_features_gui_pending_fn)(const gui08_request *request);
typedef bool (*all_features_gui_result_fn)(
    const gui08_request *request, const pc_result_v1 *result,
    const all_features_derivation_result *derivation);
typedef bool (*all_features_gui_cancelled_fn)(uint64_t request_id);

enum { ALL_FEATURES_LIFEHASH_RGB_SIZE = 32 * 32 * 3 };
typedef struct {
    uint8_t raw4[4];
    uint64_t route_epoch;
    uint64_t request_id;
    uint64_t revision;
} all_features_lifehash_request;
typedef struct {
    uint8_t rgb[ALL_FEATURES_LIFEHASH_RGB_SIZE];
    uint64_t route_epoch;
    uint64_t request_id;
    uint64_t revision;
} all_features_lifehash_result;
typedef int (*all_features_lifehash_render_fn)(
    const all_features_lifehash_request *request,
    all_features_lifehash_result *result);
typedef bool (*all_features_gui_lifehash_fn)(
    const gui08_request *request, const char fingerprint[9],
    const uint8_t *rgb, size_t rgb_size);
typedef void (*all_features_gui_lifehash_blank_fn)(void);

/* Configure before the worker starts. GUI callbacks run only on the display task;
 * lock/publish callbacks are application-owned and must remain valid for life. */
bool all_features_application_configure(
    all_features_lock_fn lock, all_features_lock_fn unlock,
    all_features_queue_publish_fn publish,
    all_features_passphrase_snapshot_fn passphrase_snapshot,
    all_features_derive_fn derive,
    all_features_gui_pending_fn pending, all_features_gui_result_fn result,
    all_features_gui_cancelled_fn cancelled);
/* Configure the display-only follow-up. Rendering runs on the worker outside
 * both transition and LVGL locks; image/blank callbacks run on the display
 * domain. This layer performs no derivation or KDF. */
bool all_features_application_lifehash_configure(
    all_features_lifehash_render_fn render,
    all_features_gui_lifehash_fn image,
    all_features_gui_lifehash_blank_fn blank);
void all_features_application_dispose(void);

/* Direct callbacks for gui_cards_bases_configure(). */
bool all_features_application_submit(const gui08_request *request);
bool all_features_application_cancel(uint64_t request_id);

/* Worker task and LVGL timer entries respectively. */
void all_features_application_work(uint64_t token, all_features_convert_fn convert);
void all_features_application_poll(void);

#ifdef __cplusplus
}
#endif
#endif
