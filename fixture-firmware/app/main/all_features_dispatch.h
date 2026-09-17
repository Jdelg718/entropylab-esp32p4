#ifndef ENTROPYLAB_ALL_FEATURES_DISPATCH_H
#define ENTROPYLAB_ALL_FEATURES_DISPATCH_H

#include "gui08_editor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ALL_FEATURES_FREE = 0,
    ALL_FEATURES_QUEUED,
    ALL_FEATURES_RUNNING,
    ALL_FEATURES_READY
} all_features_dispatch_state;

typedef struct {
    uint64_t route_epoch;
    uint64_t queue_token;
} all_features_dispatch_clock;

typedef struct {
    uint64_t route_epoch;
    uint64_t request_id;
    uint64_t revision;
    int status;
} all_features_dispatch_completion;

typedef bool (*all_features_publish_token_fn)(void *context, uint64_t token);

typedef struct {
    gui08_request request;
    all_features_dispatch_completion completion;
    uint64_t route_epoch;
    uint64_t queue_token;
    all_features_dispatch_state state;
    bool cancelled;
} all_features_dispatch;

/* Platform-neutral transition state. The application must serialize every call
 * with one lock. The queue carries only queue_token; request bytes are copied.
 * Worker compute must happen outside the transition lock and the LVGL lock. */
bool all_features_dispatch_open(all_features_dispatch *dispatch,
                                all_features_dispatch_clock *clock);
void all_features_dispatch_close(all_features_dispatch *dispatch);

bool all_features_dispatch_submit(all_features_dispatch *dispatch,
                                  all_features_dispatch_clock *clock,
                                  const gui08_request *request,
                                  all_features_publish_token_fn publish,
                                  void *publish_context,
                                  uint64_t *queue_token);

const gui08_request *all_features_dispatch_claim(all_features_dispatch *dispatch,
                                                 uint64_t queue_token);
bool all_features_dispatch_cancel(all_features_dispatch *dispatch,
                                  uint64_t queue_token);
bool all_features_dispatch_finish(all_features_dispatch *dispatch,
                                  uint64_t queue_token,
                                  const all_features_dispatch_completion *completion);
/* Locked application-owner recheck after parser conversion and before KDF. */
bool all_features_dispatch_current(const all_features_dispatch *dispatch,
                                   uint64_t queue_token,
                                   uint64_t route_epoch,
                                   uint64_t request_id,
                                   uint64_t revision);
bool all_features_dispatch_take(all_features_dispatch *dispatch,
                                uint64_t queue_token,
                                uint64_t route_epoch,
                                uint64_t request_id,
                                uint64_t revision,
                                all_features_dispatch_completion *completion);

uint64_t all_features_dispatch_route_epoch(const all_features_dispatch *dispatch);
all_features_dispatch_state all_features_dispatch_state_of(
    const all_features_dispatch *dispatch);

#ifdef __cplusplus
}
#endif
#endif
