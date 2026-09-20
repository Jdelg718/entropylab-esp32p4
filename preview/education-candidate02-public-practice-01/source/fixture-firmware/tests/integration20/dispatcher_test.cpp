#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "all_features_dispatch.h"

struct FakeQueue {
    bool accept = true;
    uint64_t published = 0;
};

static bool publish_token(void *context, uint64_t token) {
    auto *queue = static_cast<FakeQueue *>(context);
    if (!queue->accept || queue->published) return false;
    queue->published = token;
    return true;
}

static gui08_request request(uint64_t id, uint64_t revision, const char *text) {
    gui08_request value{};
    value.request_id = id;
    value.revision = revision;
    value.method = GUI08_BASES;
    value.words = 12;
    value.base = 32;
    value.passphrase_marker = GUI08_PASSPHRASE_EMPTY;
    value.length = std::strlen(text);
    std::memcpy(value.transcript, text, value.length + 1);
    return value;
}

int main() {
    all_features_dispatch_clock clock{};
    all_features_dispatch dispatch{};
    FakeQueue queue{};
    uint64_t token = 0;

    assert(all_features_dispatch_open(&dispatch, &clock));
    const uint64_t first_epoch = all_features_dispatch_route_epoch(&dispatch);
    assert(first_epoch == 1);

    auto source = request(7, 11, "qpzry");
    assert(all_features_dispatch_submit(&dispatch, &clock, &source,
                                        publish_token, &queue, &token));
    assert(token == 1 && queue.published == token);

    // Caller-owned bytes may disappear immediately after submit.
    std::memset(source.transcript, 'x', source.length);
    const gui08_request *owned = all_features_dispatch_claim(&dispatch, token);
    assert(owned && owned->request_id == 7 && owned->revision == 11);
    assert(std::strcmp(owned->transcript, "qpzry") == 0);
    assert(!all_features_dispatch_claim(&dispatch, token));

    all_features_dispatch_completion completion{};
    completion.route_epoch = first_epoch;
    completion.request_id = 7;
    completion.revision = 11;
    completion.status = 0;

    // Wrong queue token cannot affect the live request.
    assert(!all_features_dispatch_finish(&dispatch, token + 1, &completion));
    assert(all_features_dispatch_state_of(&dispatch) == ALL_FEATURES_RUNNING);

    // Matching token but stale revision is revoked, never published.
    completion.revision = 12;
    assert(!all_features_dispatch_finish(&dispatch, token, &completion));
    assert(all_features_dispatch_state_of(&dispatch) == ALL_FEATURES_FREE);

    // Queue publication failure rolls back the copied slot.
    queue = FakeQueue{false, 0};
    source = request(8, 13, "qqqqq");
    uint64_t rejected_token = 99;
    assert(!all_features_dispatch_submit(&dispatch, &clock, &source,
                                         publish_token, &queue, &rejected_token));
    assert(rejected_token == 99);
    assert(all_features_dispatch_state_of(&dispatch) == ALL_FEATURES_FREE);

    // Duplicate submit is rejected while one queue token owns the slot.
    queue = FakeQueue{};
    assert(all_features_dispatch_submit(&dispatch, &clock, &source,
                                        publish_token, &queue, &token));
    const auto duplicate = request(9, 14, "ppppp");
    assert(!all_features_dispatch_submit(&dispatch, &clock, &duplicate,
                                         publish_token, &queue, &rejected_token));
    assert(all_features_dispatch_cancel(&dispatch, token));
    assert(all_features_dispatch_state_of(&dispatch) == ALL_FEATURES_FREE);

    // Matching epoch/request/revision/token reaches take exactly once.
    queue = FakeQueue{};
    assert(all_features_dispatch_submit(&dispatch, &clock, &source,
                                        publish_token, &queue, &token));
    owned = all_features_dispatch_claim(&dispatch, token);
    assert(owned);
    completion = {first_epoch, source.request_id, source.revision, -7};
    assert(all_features_dispatch_finish(&dispatch, token, &completion));
    all_features_dispatch_completion output{};
    assert(!all_features_dispatch_take(&dispatch, token, first_epoch,
                                       source.request_id, source.revision + 1, &output));
    assert(all_features_dispatch_state_of(&dispatch) == ALL_FEATURES_FREE);

    queue = FakeQueue{};
    assert(all_features_dispatch_submit(&dispatch, &clock, &source,
                                        publish_token, &queue, &token));
    assert(all_features_dispatch_claim(&dispatch, token));
    completion = {first_epoch, source.request_id, source.revision, 0};
    assert(all_features_dispatch_finish(&dispatch, token, &completion));
    assert(all_features_dispatch_take(&dispatch, token, first_epoch,
                                      source.request_id, source.revision, &output));
    assert(output.status == 0);
    assert(!all_features_dispatch_take(&dispatch, token, first_epoch,
                                       source.request_id, source.revision, &output));

    // Route teardown revokes an old queued token; reopening gets a new epoch.
    queue = FakeQueue{};
    assert(all_features_dispatch_submit(&dispatch, &clock, &source,
                                        publish_token, &queue, &token));
    all_features_dispatch_close(&dispatch);
    assert(all_features_dispatch_state_of(&dispatch) == ALL_FEATURES_FREE);
    assert(all_features_dispatch_route_epoch(&dispatch) == 0);
    assert(!all_features_dispatch_claim(&dispatch, token));
    assert(all_features_dispatch_open(&dispatch, &clock));
    assert(all_features_dispatch_route_epoch(&dispatch) > first_epoch);

    // Exhaustion never wraps into a reusable identity.
    all_features_dispatch_close(&dispatch);
    clock.route_epoch = UINT64_MAX;
    assert(!all_features_dispatch_open(&dispatch, &clock));

    std::puts("integration20 dispatcher copy/revision/token routing PASS");
    return 0;
}
