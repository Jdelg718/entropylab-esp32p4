#include "all_features_dispatch.h"

#include <climits>
#include <cstddef>
#include <cstring>

namespace {

void wipe(void *memory, size_t size) {
    volatile unsigned char *byte = static_cast<volatile unsigned char *>(memory);
    while (size--) *byte++ = 0;
}

void release_slot(all_features_dispatch *dispatch) {
    const uint64_t route_epoch = dispatch->route_epoch;
    wipe(dispatch, sizeof(*dispatch));
    dispatch->route_epoch = route_epoch;
}

bool valid_words(unsigned words) {
    return words == 12 || words == 15 || words == 18 || words == 21 || words == 24;
}

bool valid_request(const gui08_request *request) {
    if (!request || !request->request_id || !request->revision ||
        !valid_words(request->words) || request->length > 1024 ||
        request->passphrase_len > sizeof(request->passphrase) ||
        request->transcript[request->length] != '\0') {
        return false;
    }
    if (request->method == GUI08_SEED) {
        if ((request->seed_operation != 1 && request->seed_operation != 2) ||
            request->base > 1) {
            return false;
        }
    } else if (request->method == GUI08_DICE) {
        if ((request->dice_method != GUI08_DICE_BITBOX &&
             request->dice_method != GUI08_DICE_DPLUS) ||
            request->dice_final_length >= sizeof(request->dice_final) ||
            request->dice_final[request->dice_final_length] != '\0') {
            return false;
        }
    } else if ((request->method == GUI08_CARDS ||
                request->method == GUI08_BASES) &&
               request->seed_operation == 0) {
        /* Existing Cards/Bases request shape. */
    } else {
        return false;
    }
    return (request->passphrase_marker == GUI08_PASSPHRASE_EMPTY &&
            request->passphrase_len == 0) ||
           (request->passphrase_marker == GUI08_PASSPHRASE_ACTIVE &&
            request->passphrase_len != 0);
}

bool identity_matches(const all_features_dispatch *dispatch,
                      const all_features_dispatch_completion *completion) {
    return completion && completion->route_epoch == dispatch->route_epoch &&
           completion->request_id == dispatch->request.request_id &&
           completion->revision == dispatch->request.revision;
}

}  // namespace

extern "C" bool all_features_dispatch_open(all_features_dispatch *dispatch,
                                            all_features_dispatch_clock *clock) {
    if (!dispatch || !clock || dispatch->route_epoch ||
        dispatch->state != ALL_FEATURES_FREE || clock->route_epoch == UINT64_MAX) {
        return false;
    }
    dispatch->route_epoch = ++clock->route_epoch;
    return true;
}

extern "C" void all_features_dispatch_close(all_features_dispatch *dispatch) {
    if (!dispatch) return;
    dispatch->route_epoch = 0;
    if (dispatch->state == ALL_FEATURES_RUNNING) {
        dispatch->cancelled = true;
        return;
    }
    release_slot(dispatch);
}

extern "C" bool all_features_dispatch_submit(
    all_features_dispatch *dispatch, all_features_dispatch_clock *clock,
    const gui08_request *request, all_features_publish_token_fn publish,
    void *publish_context, uint64_t *queue_token) {
    if (!dispatch || !clock || !publish || !queue_token ||
        dispatch->state != ALL_FEATURES_FREE || !dispatch->route_epoch ||
        clock->queue_token == UINT64_MAX || !valid_request(request)) {
        return false;
    }

    const uint64_t token = ++clock->queue_token;
    std::memcpy(&dispatch->request, request, sizeof(*request));
    dispatch->queue_token = token;
    dispatch->state = ALL_FEATURES_QUEUED;

    if (!publish(publish_context, token)) {
        release_slot(dispatch);
        return false;
    }

    *queue_token = token;
    return true;
}

extern "C" const gui08_request *all_features_dispatch_claim(
    all_features_dispatch *dispatch, uint64_t queue_token) {
    if (!dispatch || !queue_token || dispatch->queue_token != queue_token ||
        dispatch->state != ALL_FEATURES_QUEUED || !dispatch->route_epoch) {
        return nullptr;
    }
    dispatch->state = ALL_FEATURES_RUNNING;
    return &dispatch->request;
}

extern "C" bool all_features_dispatch_cancel(all_features_dispatch *dispatch,
                                              uint64_t queue_token) {
    if (!dispatch || !queue_token || dispatch->queue_token != queue_token ||
        dispatch->state == ALL_FEATURES_FREE) {
        return false;
    }
    if (dispatch->state == ALL_FEATURES_RUNNING) {
        dispatch->cancelled = true;
    } else {
        release_slot(dispatch);
    }
    return true;
}

extern "C" bool all_features_dispatch_finish(
    all_features_dispatch *dispatch, uint64_t queue_token,
    const all_features_dispatch_completion *completion) {
    if (!dispatch || !queue_token || dispatch->queue_token != queue_token ||
        dispatch->state != ALL_FEATURES_RUNNING) {
        return false;
    }
    if (dispatch->cancelled || !identity_matches(dispatch, completion)) {
        release_slot(dispatch);
        return false;
    }
    dispatch->completion = *completion;
    wipe(&dispatch->request, sizeof(dispatch->request));
    dispatch->state = ALL_FEATURES_READY;
    return true;
}

extern "C" bool all_features_dispatch_current(
    const all_features_dispatch *dispatch, uint64_t queue_token,
    uint64_t route_epoch, uint64_t request_id, uint64_t revision) {
    return dispatch && dispatch->state == ALL_FEATURES_RUNNING &&
           !dispatch->cancelled && dispatch->route_epoch == route_epoch &&
           dispatch->queue_token == queue_token &&
           dispatch->request.request_id == request_id &&
           dispatch->request.revision == revision;
}

extern "C" bool all_features_dispatch_take(
    all_features_dispatch *dispatch, uint64_t queue_token, uint64_t route_epoch,
    uint64_t request_id, uint64_t revision,
    all_features_dispatch_completion *completion) {
    if (!completion) return false;
    wipe(completion, sizeof(*completion));
    if (!dispatch || !queue_token || dispatch->queue_token != queue_token ||
        dispatch->state != ALL_FEATURES_READY) {
        return false;
    }

    const bool matches = dispatch->route_epoch &&
                         dispatch->completion.route_epoch == route_epoch &&
                         dispatch->completion.request_id == request_id &&
                         dispatch->completion.revision == revision;
    if (matches) *completion = dispatch->completion;
    release_slot(dispatch);
    return matches;
}

extern "C" uint64_t all_features_dispatch_route_epoch(
    const all_features_dispatch *dispatch) {
    return dispatch ? dispatch->route_epoch : 0;
}

extern "C" all_features_dispatch_state all_features_dispatch_state_of(
    const all_features_dispatch *dispatch) {
    return dispatch ? dispatch->state : ALL_FEATURES_FREE;
}
