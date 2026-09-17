#include "all_features_application.h"

#include <cstring>

namespace {

struct ApplicationState {
    all_features_dispatch dispatch;
    all_features_dispatch_clock clock;
    all_features_lock_fn lock;
    all_features_lock_fn unlock;
    all_features_queue_publish_fn publish;
    all_features_passphrase_snapshot_fn passphrase_snapshot;
    all_features_derive_fn derive;
    all_features_gui_pending_fn pending;
    all_features_gui_result_fn result;
    all_features_gui_cancelled_fn cancelled;
    all_features_lifehash_render_fn lifehash_render;
    all_features_gui_lifehash_fn lifehash_image;
    all_features_gui_lifehash_blank_fn lifehash_blank;
    gui08_request context;
    pc_result_v1 conversion;
    all_features_derivation_result derivation;
    all_features_lifehash_result lifehash;
    uint64_t token;
    bool configured;
};

ApplicationState app;

void wipe(void *memory, size_t size) {
    volatile unsigned char *byte = static_cast<volatile unsigned char *>(memory);
    while (size--) *byte++ = 0;
}

bool publish_adapter(void *, uint64_t token) {
    /* Normative erratum: the token-only queue call is outside the transition
     * lock. all_features_dispatch_submit resumes locked for exact rollback. */
    app.unlock();
    const bool published = app.publish(token);
    app.lock();
    return published;
}

int hex_nibble(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

bool prepare_lifehash(const all_features_derivation_result &derivation,
                      uint64_t route_epoch, const gui08_request &request,
                      all_features_lifehash_request *output) {
    if (!output || derivation.fingerprint[8] != '\0') return false;
    all_features_lifehash_request owned{};
    for (size_t i = 0; i < sizeof owned.raw4; ++i) {
        const int high = hex_nibble(derivation.fingerprint[i * 2]);
        const int low = hex_nibble(derivation.fingerprint[i * 2 + 1]);
        if (high < 0 || low < 0) return false;
        owned.raw4[i] = static_cast<uint8_t>((high << 4) | low);
    }
    owned.route_epoch = route_epoch;
    owned.request_id = request.request_id;
    owned.revision = request.revision;
    *output = owned;
    return true;
}

bool lifehash_identity_matches(const all_features_lifehash_result &result,
                               const all_features_lifehash_request &request) {
    return result.route_epoch == request.route_epoch &&
           result.request_id == request.request_id &&
           result.revision == request.revision;
}

void reset_context() {
    wipe(&app.context, sizeof(app.context));
    wipe(&app.conversion, sizeof(app.conversion));
    wipe(&app.derivation, sizeof(app.derivation));
    wipe(&app.lifehash, sizeof(app.lifehash));
    app.token = 0;
}

}  // namespace

extern "C" bool all_features_application_configure(
    all_features_lock_fn lock, all_features_lock_fn unlock,
    all_features_queue_publish_fn publish,
    all_features_passphrase_snapshot_fn passphrase_snapshot,
    all_features_derive_fn derive,
    all_features_gui_pending_fn pending, all_features_gui_result_fn result,
    all_features_gui_cancelled_fn cancelled) {
    if (app.configured || !lock || !unlock || !publish || !passphrase_snapshot ||
        !derive || !pending || !result || !cancelled) return false;
    app.lock = lock;
    app.unlock = unlock;
    app.publish = publish;
    app.passphrase_snapshot = passphrase_snapshot;
    app.derive = derive;
    app.pending = pending;
    app.result = result;
    app.cancelled = cancelled;
    app.lock();
    const bool opened = all_features_dispatch_open(&app.dispatch, &app.clock);
    app.unlock();
    if (!opened) {
        wipe(&app, sizeof(app));
        return false;
    }
    app.configured = true;
    return true;
}

extern "C" bool all_features_application_lifehash_configure(
    all_features_lifehash_render_fn render, all_features_gui_lifehash_fn image,
    all_features_gui_lifehash_blank_fn blank) {
    if (!app.configured || app.lifehash_render || !render || !image || !blank)
        return false;
    app.lifehash_render = render;
    app.lifehash_image = image;
    app.lifehash_blank = blank;
    return true;
}

extern "C" void all_features_application_dispose(void) {
    if (!app.configured) return;
    if (app.lifehash_blank) app.lifehash_blank();
    app.lock();
    all_features_dispatch_close(&app.dispatch);
    reset_context();
    app.unlock();
    wipe(&app, sizeof(app));
}

extern "C" bool all_features_application_submit(const gui08_request *request) {
    if (!app.configured || !request) return false;
    gui08_request owned = *request;
    wipe(owned.passphrase, sizeof(owned.passphrase));
    owned.passphrase_len = 0;
    owned.passphrase_marker = app.passphrase_snapshot(
        request, owned.passphrase, &owned.passphrase_len);
    const bool valid_passphrase = owned.passphrase_len <= sizeof(owned.passphrase) &&
        ((owned.passphrase_marker == GUI08_PASSPHRASE_EMPTY && owned.passphrase_len == 0) ||
         (owned.passphrase_marker == GUI08_PASSPHRASE_ACTIVE && owned.passphrase_len != 0));
    if (!valid_passphrase) {
        wipe(&owned, sizeof(owned));
        return false;
    }

    if (app.lifehash_blank) app.lifehash_blank();
    uint64_t token = 0;
    app.lock();
    const bool published = all_features_dispatch_submit(
        &app.dispatch, &app.clock, &owned, publish_adapter, nullptr, &token);
    if (published) {
        app.context = owned;
        app.token = token;
    }
    app.unlock();

    const bool marked = published && app.pending(&owned);
    if (published && !marked) {
        app.lock();
        (void)all_features_dispatch_cancel(&app.dispatch, token);
        reset_context();
        app.unlock();
        if (app.lifehash_blank) app.lifehash_blank();
    }
    wipe(&owned, sizeof(owned));
    return marked;
}

extern "C" bool all_features_application_cancel(uint64_t request_id) {
    if (!app.configured || !request_id) return false;
    app.lock();
    const bool matches = app.context.request_id == request_id && app.token != 0;
    const uint64_t token = app.token;
    const bool was_cancelled = matches && all_features_dispatch_cancel(&app.dispatch, token);
    if (was_cancelled && all_features_dispatch_state_of(&app.dispatch) == ALL_FEATURES_FREE)
        reset_context();
    app.unlock();
    if (was_cancelled && app.lifehash_blank) app.lifehash_blank();
    return was_cancelled;
}

extern "C" void all_features_application_work(uint64_t token,
                                                all_features_convert_fn convert) {
    if (!app.configured || !convert || !token) return;
    gui08_request request{};
    uint64_t route_epoch = 0;
    app.lock();
    const gui08_request *claimed = all_features_dispatch_claim(&app.dispatch, token);
    if (claimed) {
        request = *claimed;
        route_epoch = all_features_dispatch_route_epoch(&app.dispatch);
    }
    app.unlock();
    if (!claimed) return;

    pc_result_v1 conversion{};
    all_features_derivation_result derivation{};
    all_features_lifehash_request lifehash_request{};
    all_features_lifehash_result lifehash_result{};
    int status = convert(&request, &conversion);
    if (status == 0) {
        app.lock();
        const bool current = all_features_dispatch_current(
            &app.dispatch, token, route_epoch, request.request_id, request.revision);
        app.unlock();
        status = current ? app.derive(&request, &conversion, &derivation) : -1;
    }
    if (status == 0 && app.lifehash_render) {
        if (!prepare_lifehash(derivation, route_epoch, request, &lifehash_request)) {
            status = -1;
        } else {
            lifehash_result.route_epoch = route_epoch;
            lifehash_result.request_id = request.request_id;
            lifehash_result.revision = request.revision;
            status = app.lifehash_render(&lifehash_request, &lifehash_result);
            if (status == 0 && !lifehash_identity_matches(lifehash_result, lifehash_request))
                status = -1;
        }
    }
    all_features_dispatch_completion completion{
        route_epoch, request.request_id, request.revision, status};
    app.lock();
    const bool finished = all_features_dispatch_finish(&app.dispatch, token, &completion);
    if (finished && status == 0) {
        app.conversion = conversion;
        app.derivation = derivation;
        app.lifehash = lifehash_result;
    }
    app.unlock();
    wipe(&request, sizeof(request));
    wipe(&conversion, sizeof(conversion));
    wipe(&derivation, sizeof(derivation));
    wipe(&lifehash_request, sizeof(lifehash_request));
    wipe(&lifehash_result, sizeof(lifehash_result));
    wipe(&completion, sizeof(completion));
}

extern "C" void all_features_application_poll(void) {
    if (!app.configured) return;
    gui08_request context{};
    all_features_dispatch_completion completion{};
    pc_result_v1 conversion{};
    all_features_derivation_result derivation{};
    all_features_lifehash_result lifehash{};
    bool taken = false;
    app.lock();
    if (app.token) {
        context = app.context;
        taken = all_features_dispatch_take(
            &app.dispatch, app.token, all_features_dispatch_route_epoch(&app.dispatch),
            context.request_id, context.revision, &completion);
        if (taken) {
            conversion = app.conversion;
            derivation = app.derivation;
            lifehash = app.lifehash;
            reset_context();
        }
    }
    app.unlock();
    if (taken) {
        if (completion.status == 0) {
            const bool accepted = app.result(&context, &conversion, &derivation);
            const bool image_accepted = accepted && app.lifehash_image &&
                app.lifehash_image(&context, derivation.fingerprint,
                                   lifehash.rgb, sizeof(lifehash.rgb));
            if (!image_accepted && app.lifehash_blank) app.lifehash_blank();
        } else {
            if (app.lifehash_blank) app.lifehash_blank();
            (void)app.cancelled(context.request_id);
        }
    }
    wipe(&context, sizeof(context));
    wipe(&conversion, sizeof(conversion));
    wipe(&derivation, sizeof(derivation));
    wipe(&lifehash, sizeof(lifehash));
    wipe(&completion, sizeof(completion));
}
