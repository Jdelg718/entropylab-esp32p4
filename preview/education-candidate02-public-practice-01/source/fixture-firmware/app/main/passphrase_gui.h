#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"
#include "fingerprint_view.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { PASSPHRASE_UI_RAW_MAX = 256, PASSPHRASE_UI_KEY_MAX = 35 };
typedef enum { PASSPHRASE_UI_EMPTY=0, PASSPHRASE_UI_ACTIVE=1 } passphrase_ui_marker_t;
typedef enum { PASSPHRASE_UI_OK=0, PASSPHRASE_UI_ERR_LIMIT=-1, PASSPHRASE_UI_ERR_UTF8=-2, PASSPHRASE_UI_ERR_EDITOR_UNSUPPORTED=-3, PASSPHRASE_UI_ERR_BUSY=-4, PASSPHRASE_UI_ERR_INSTANCE=-5 } passphrase_ui_rc_t;
typedef struct { uint32_t mode; uint64_t source_revision; uint32_t word_count; uint32_t derivation_context; } passphrase_ui_context_t;
typedef struct {
    uint8_t raw[PASSPHRASE_UI_RAW_MAX]; size_t raw_len;
    passphrase_ui_marker_t marker; passphrase_ui_context_t context;
    uint64_t request_id, generation;
} passphrase_ui_request_t;
typedef struct {
    uint64_t request_id, generation; passphrase_ui_context_t context;
    passphrase_ui_marker_t marker; int32_t rc;
    char fingerprint[9]; char address[43];
} passphrase_ui_completion_t;
typedef bool (*passphrase_ui_submit_cb)(const passphrase_ui_request_t *, void *user);
typedef enum {
    PASSPHRASE_UI_CONTROL_REVEAL, PASSPHRASE_UI_CONTROL_BACK,
    PASSPHRASE_UI_CONTROL_CLEAR, PASSPHRASE_UI_CONTROL_CONTINUE,
    PASSPHRASE_UI_CONTROL_KEEP_MASKED, PASSPHRASE_UI_CONTROL_REVEAL_NOW,
    PASSPHRASE_UI_CONTROL_COUNT
} passphrase_ui_control_t;
typedef struct passphrase_ui {
    lv_obj_t *root,*viewport,*content,*field,*field_text,*status,*counter,*context_label,*result,*result_marker_label,*result_fp,*result_addr,*modal,*keyboard;
    lv_obj_t *controls[PASSPHRASE_UI_CONTROL_COUNT],*keys[PASSPHRASE_UI_KEY_MAX],*focus_target;
    bool live, disposing;
    unsigned page; lv_group_t *group;
    lv_style_t base,card,well,control,primary,disabled,focus;
    uint8_t raw[PASSPHRASE_UI_RAW_MAX]; size_t raw_len;
    passphrase_ui_context_t context; bool context_set,masked,revealed,pending,result_visible,modal_open,keyboard_open,shift;
    passphrase_ui_marker_t result_marker; uint64_t next_id,generation,pending_id,pending_generation;
    passphrase_ui_context_t pending_context; passphrase_ui_marker_t pending_marker;
    passphrase_ui_submit_cb submit; void *submit_user;
    fingerprint_view lifehash;
} passphrase_ui_t;

/* Lifetime/serialization contract (standalone, no worker implementation):
 * - Declare storage as passphrase_ui_t ui = {0} BEFORE its first API call.
 *   Never pass uninitialized storage, copy/move a live owner, or modify fields.
 *   Keep the owner alive and stationary until synchronous root/parent deletion
 *   or destroy returns. All APIs and LVGL operations run serialized on the GUI
 *   thread under the integration's display-lock discipline.
 * - init(NULL,...) is a no-op; init on a live owner is a no-op (not replacement).
 *   NULL parent follows LVGL screen creation semantics. After destroy/external
 *   deletion, init may reuse the SAME owner without memset; counters survive.
 * - destroy is synchronous and repeatable. External root or ancestor deletion
 *   uses the same cleanup: invalidate pending, best-effort full draft wipe,
 *   delete children while owner/styles live, detach root styles, delete focus
 *   group (LVGL detaches input devices), reset styles and clear object handles.
 *   Do not delete individual children or register teardown-reentrant callbacks.
 * - Integration MUST cancel/detach completion routing BEFORE destroy, ancestor
 *   deletion, or freeing/replacing storage, and drain queued callbacks before
 *   reuse. No callback may address freed storage. Preserved counters reject old
 *   requests on same-owner reinit; they do NOT replace lifetime-aware routing
 *   for a fresh zeroed owner, address reuse, or counter wrap. Worker cancellation
 *   and any copies already owned by the worker remain integration-owned.
 * - submit must synchronously COPY the entire request before returning true;
 *   never retain its stack pointer. false means no accepted asynchronous job.
 *   submit must not reenter any UI API, delete objects, or tear down the owner.
 * - NULL / zero-initialized / disposed instances: mutators are no-ops, setters
 *   return ERR_INSTANCE, complete returns false; pointer accessors return NULL,
 *   counts zero, markers Empty, flags false except is_masked returns true.
 * - raw is a borrowed pointer until mutation/disposal. set_raw accepts bounded
 *   whole/interior aliases and preserves the draft on invalid input. Caller must
 *   supply n readable bytes; validation does not make arbitrary pointers safe.
 * Best-effort clearing is NOT secure erasure of allocator/framebuffer/stack or
 * worker copies. No crypto, queue, persistence or hardware assurance is implied.
 */
void passphrase_ui_init(passphrase_ui_t *, lv_obj_t *parent, passphrase_ui_submit_cb, void *user);
void passphrase_ui_destroy(passphrase_ui_t *);
void passphrase_ui_set_context(passphrase_ui_t *, passphrase_ui_context_t);
passphrase_ui_rc_t passphrase_ui_set_raw(passphrase_ui_t *, const uint8_t *, size_t);
passphrase_ui_rc_t passphrase_ui_editor_append(passphrase_ui_t *, uint32_t ascii);
void passphrase_ui_clear(passphrase_ui_t *); /* best-effort UI clear; not secure erasure */
void passphrase_ui_cancel(passphrase_ui_t *); /* explicit Cancel destroys the draft */
void passphrase_ui_back(passphrase_ui_t *);   /* same-context draft retained, masked */
bool passphrase_ui_complete(passphrase_ui_t *, const passphrase_ui_completion_t *);
bool passphrase_ui_lifehash_publish(passphrase_ui_t *, const char fingerprint[9],
                                    const uint8_t *rgb, size_t rgb_size);
void passphrase_ui_lifehash_rendering(passphrase_ui_t *);
void passphrase_ui_lifehash_failed(passphrase_ui_t *);
bool passphrase_ui_lifehash_visible(const passphrase_ui_t *);
void passphrase_ui_open_keyboard(passphrase_ui_t *);
const uint8_t *passphrase_ui_raw(const passphrase_ui_t *);
size_t passphrase_ui_raw_len(const passphrase_ui_t *);
passphrase_ui_marker_t passphrase_ui_snapshot_marker(const passphrase_ui_t *);
bool passphrase_ui_is_masked(const passphrase_ui_t *);
bool passphrase_ui_pending(const passphrase_ui_t *);
bool passphrase_ui_result_visible(const passphrase_ui_t *);
passphrase_ui_marker_t passphrase_ui_result_marker(const passphrase_ui_t *);
bool passphrase_ui_modal_open(const passphrase_ui_t *);
bool passphrase_ui_keyboard_open(const passphrase_ui_t *);
const char *passphrase_ui_status(const passphrase_ui_t *);
lv_obj_t *passphrase_ui_root(const passphrase_ui_t *);
lv_obj_t *passphrase_ui_control(const passphrase_ui_t *, passphrase_ui_control_t);
lv_obj_t *passphrase_ui_focus_target(const passphrase_ui_t *);
unsigned passphrase_ui_keyboard_key_count(const passphrase_ui_t *);
lv_obj_t *passphrase_ui_keyboard_key(const passphrase_ui_t *, unsigned);

#ifdef __cplusplus
}
#endif
