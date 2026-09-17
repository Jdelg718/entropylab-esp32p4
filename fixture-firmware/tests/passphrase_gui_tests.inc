/* Real LVGL behavioral tests. Public fixture strings only; not crypto/queue/hardware evidence. */
#include "passphrase_gui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned pp_submits;
static bool pp_accept = true;
static passphrase_ui_request_t pp_owned_request;
static bool pp_submit_stub(const passphrase_ui_request_t *r, void *user) {
    (void)user; pp_submits++; pp_owned_request = *r; return pp_accept;
}
static void pp_click(lv_obj_t *o) { assert(o); assert(!lv_obj_has_state(o, LV_STATE_DISABLED)); lv_obj_send_event(o, LV_EVENT_CLICKED, NULL); }
static void pp_assert_target(lv_obj_t *o) { assert(o); lv_obj_update_layout(lv_screen_active()); assert(lv_obj_get_width(o) >= 44); assert(lv_obj_get_height(o) >= 44); }
/* Real LVGL pointer driver: press/release traverses hit testing, not send_event. */
static lv_point_t pp_point; static bool pp_pressed; static lv_indev_t *pp_pointer;
static void pp_read(lv_indev_t *indev,lv_indev_data_t *data){(void)indev;data->point=pp_point;data->state=pp_pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
static void pp_touch(lv_obj_t *o){lv_obj_update_layout(lv_screen_active());lv_area_t a;lv_obj_get_coords(o,&a);pp_point.x=(a.x1+a.x2)/2;pp_point.y=(a.y1+a.y2)/2;pp_pressed=true;lv_indev_read(pp_pointer);lv_tick_inc(40);pp_pressed=false;lv_indev_read(pp_pointer);lv_tick_inc(40);}
static bool pp_intersects(lv_area_t a,lv_area_t b){return a.x1<=b.x2&&b.x1<=a.x2&&a.y1<=b.y2&&b.y1<=a.y2;}
static void pp_visible_bounds(lv_obj_t *o){lv_area_t a,b;lv_obj_get_coords(o,&a);assert(a.x1>=0&&a.y1>=0&&a.x2<480&&a.y2<800);for(lv_obj_t *p=lv_obj_get_parent(o);p;p=lv_obj_get_parent(p)){assert(!lv_obj_has_flag(p,LV_OBJ_FLAG_HIDDEN));lv_obj_get_coords(p,&b);assert(a.x1>=b.x1&&a.y1>=b.y1&&a.x2<=b.x2&&a.y2<=b.y2);}}
static void pp_layout(passphrase_ui_t *u){
 lv_obj_update_layout(u->root);lv_obj_t *objects[48];unsigned n=0;
 objects[n++]=u->field;objects[n++]=u->counter;objects[n++]=u->status;objects[n++]=u->context_label;
 for(unsigned i=0;i<4;i++)objects[n++]=u->controls[i];
 for(unsigned i=0;i<PASSPHRASE_UI_KEY_MAX;i++)if(!lv_obj_has_flag(u->keys[i],LV_OBJ_FLAG_HIDDEN)){pp_assert_target(u->keys[i]);objects[n++]=u->keys[i];}
 for(unsigned i=0;i<n;i++){pp_visible_bounds(objects[i]);for(unsigned j=i+1;j<n;j++){lv_area_t a,b;lv_obj_get_coords(objects[i],&a);lv_obj_get_coords(objects[j],&b);assert(!pp_intersects(a,b));}}
}
static passphrase_ui_context_t pp_context(uint32_t mode, uint64_t revision, uint32_t words) {
    passphrase_ui_context_t c = {.mode=mode,.source_revision=revision,.word_count=words,.derivation_context=1}; return c;
}
static void pp_inert_api_tests(passphrase_ui_t *u) {
    passphrase_ui_set_context(u,pp_context(0,0,0));passphrase_ui_clear(u);
    passphrase_ui_cancel(u);passphrase_ui_back(u);passphrase_ui_open_keyboard(u);
    passphrase_ui_destroy(u);
    assert(passphrase_ui_set_raw(u,(const uint8_t *)"A",1)==PASSPHRASE_UI_ERR_INSTANCE);
    assert(passphrase_ui_editor_append(u,'A')==PASSPHRASE_UI_ERR_INSTANCE);
    passphrase_ui_completion_t r={0};assert(!passphrase_ui_complete(u,&r));
    assert(!passphrase_ui_raw(u) && !passphrase_ui_raw_len(u));
    assert(passphrase_ui_snapshot_marker(u)==PASSPHRASE_UI_EMPTY);
    assert(passphrase_ui_result_marker(u)==PASSPHRASE_UI_EMPTY);
    assert(passphrase_ui_is_masked(u) && !passphrase_ui_pending(u));
    assert(!passphrase_ui_result_visible(u) && !passphrase_ui_modal_open(u));
    assert(!passphrase_ui_keyboard_open(u) && !passphrase_ui_status(u));
    assert(!passphrase_ui_root(u) && !passphrase_ui_control(u,0));
    assert(!passphrase_ui_focus_target(u) && !passphrase_ui_keyboard_key_count(u));
    assert(!passphrase_ui_keyboard_key(u,0));
}
static unsigned pp_deleted_children;
static void pp_child_delete_probe(lv_event_t *e) {
    passphrase_ui_t *u=lv_event_get_user_data(e);lv_style_value_t value;
    assert(u->disposing && !u->live && !u->pending);
    assert(lv_style_get_prop(&u->control,LV_STYLE_BG_COLOR,&value)==LV_STYLE_RES_FOUND);
    for(unsigned i=0;i<256;i++)assert(u->raw[i]==0);
    passphrase_ui_completion_t r={0};assert(!passphrase_ui_complete(u,&r));
    pp_deleted_children++;
}
static void pp_lifetime_tests(void) {
    passphrase_ui_init(NULL,NULL,NULL,NULL);pp_inert_api_tests(NULL);
    passphrase_ui_t u={0};pp_inert_api_tests(&u);
    lv_obj_t *parent=lv_obj_create(lv_screen_active());
    passphrase_ui_init(&u,parent,pp_submit_stub,NULL);
    assert(passphrase_ui_set_raw(&u,(const uint8_t *)"PublicLifetime",14)==0);
    pp_click(u.controls[PASSPHRASE_UI_CONTROL_CONTINUE]);
    passphrase_ui_completion_t old={.request_id=pp_owned_request.request_id,.generation=pp_owned_request.generation,.context=pp_owned_request.context,.marker=pp_owned_request.marker};
    lv_obj_delete(parent);
    for(unsigned i=0;i<256;i++)assert(u.raw[i]==0);
    assert(!u.root && !u.group && !u.pending && !u.controls[0] && !u.keys[0]);
    lv_style_value_t value;assert(lv_style_get_prop(&u.base,LV_STYLE_BG_COLOR,&value)!=LV_STYLE_RES_FOUND);
    assert(!passphrase_ui_complete(&u,&old));
    pp_inert_api_tests(&u);
    lv_indev_t *keyboard=lv_indev_create();lv_indev_set_type(keyboard,LV_INDEV_TYPE_KEYPAD);
    for(unsigned cycle=0;cycle<16;cycle++) {
        passphrase_ui_init(&u,lv_screen_active(),pp_submit_stub,NULL);
        lv_obj_t *root=u.root;passphrase_ui_init(&u,lv_screen_active(),NULL,NULL);
        assert(u.root==root && u.submit==pp_submit_stub);
        lv_indev_set_group(keyboard,u.group);
        assert(passphrase_ui_set_raw(&u,(const uint8_t *)"PublicLifetime",14)==0);
        pp_click(u.controls[PASSPHRASE_UI_CONTROL_CONTINUE]);
        assert(!passphrase_ui_complete(&u,&old));
        old=(passphrase_ui_completion_t){.request_id=pp_owned_request.request_id,.generation=pp_owned_request.generation,.context=pp_owned_request.context,.marker=pp_owned_request.marker};
        lv_obj_add_event_cb(u.field,pp_child_delete_probe,LV_EVENT_DELETE,&u);
        if(cycle%2)passphrase_ui_destroy(&u);else lv_obj_delete(u.root);
        assert(!lv_indev_get_group(keyboard));
        assert(!u.root && !u.group && !u.submit && !u.submit_user && !u.focus_target);
        lv_style_t *styles[]={&u.base,&u.card,&u.well,&u.control,&u.primary,&u.disabled,&u.focus};
        for(unsigned j=0;j<7;j++)assert(lv_style_is_empty(styles[j]));
        assert(!passphrase_ui_complete(&u,&old));pp_inert_api_tests(&u);
    }
    assert(pp_deleted_children==16);lv_indev_delete(keyboard);
    puts("PASS B2 external parent/root/dispose, 16 reinit cycles, child style lifetime, indev detach, stale completion isolation; all null/inert APIs");
}
static void passphrase_gui_tests(void) {
    pp_lifetime_tests();
    static passphrase_ui_t ui; /* LVGL retains style references through host render. */
    passphrase_ui_init(&ui, lv_screen_active(), pp_submit_stub, NULL);
    passphrase_ui_set_context(&ui, pp_context(0, 7, 12));
    /* B1: public accessor self-assignment and interior overlap preserve identity. */
    assert(passphrase_ui_set_raw(&ui,(const uint8_t *)"PublicAlias",11)==0);
    assert(passphrase_ui_set_raw(&ui,passphrase_ui_raw(&ui),11)==0);
    assert(!memcmp(passphrase_ui_raw(&ui),"PublicAlias",11));
    assert(passphrase_ui_set_raw(&ui,passphrase_ui_raw(&ui)+6,5)==0);
    assert(ui.raw_len==5 && !memcmp(ui.raw,"Alias",5));
    uint8_t alias_max[256];memset(alias_max,'Q',sizeof alias_max);
    assert(passphrase_ui_set_raw(&ui,alias_max,256)==0);
    assert(passphrase_ui_set_raw(&ui,passphrase_ui_raw(&ui),256)==0);
    assert(ui.raw_len==256 && !memcmp(ui.raw,alias_max,256));
    assert(passphrase_ui_set_raw(&ui,passphrase_ui_raw(&ui),257)==PASSPHRASE_UI_ERR_LIMIT);
    assert(!memcmp(ui.raw,alias_max,256));
    const uint8_t alias_utf8[]={0xc3,0xa9,'Z'};
    assert(passphrase_ui_set_raw(&ui,alias_utf8,3)==0);
    assert(passphrase_ui_set_raw(&ui,passphrase_ui_raw(&ui)+1,2)==PASSPHRASE_UI_ERR_UTF8);
    assert(ui.raw_len==3 && !memcmp(ui.raw,alias_utf8,3));
    /* Reviewer validation probes retained as public regression fixtures. */
    const uint8_t malformed[][4]={{0},{0xc0,0xaf},{0xed,0xa0,0x80},{0xf4,0x90,0x80,0x80},{0xf0,0x80,0x80,0x80},{0x80}};
    const size_t malformed_sizes[]={1,2,3,4,4,1};
    for(unsigned i=0;i<6;i++) {
        assert(passphrase_ui_set_raw(&ui,malformed[i],malformed_sizes[i])==PASSPHRASE_UI_ERR_UTF8);
        assert(ui.raw_len==3 && !memcmp(ui.raw,alias_utf8,3));
    }
    assert(passphrase_ui_set_raw(&ui,NULL,1)==PASSPHRASE_UI_ERR_UTF8);
    assert(passphrase_ui_set_raw(&ui,NULL,SIZE_MAX)==PASSPHRASE_UI_ERR_LIMIT);
    assert(passphrase_ui_set_raw(&ui,NULL,0)==0);
    assert(!passphrase_ui_keyboard_key(&ui,(unsigned)-1));
    assert(passphrase_ui_set_raw(&ui,alias_max,256)==0);
    pp_click(ui.controls[PASSPHRASE_UI_CONTROL_CONTINUE]);unsigned held_page=ui.page;
    lv_obj_send_event(ui.keys[34],LV_EVENT_CLICKED,NULL);assert(ui.page==held_page);
    passphrase_ui_completion_t alias_old={.request_id=pp_owned_request.request_id,.generation=pp_owned_request.generation,.context=pp_owned_request.context,.marker=pp_owned_request.marker};
    assert(passphrase_ui_set_raw(&ui,(const uint8_t *)"New",3)==0);
    assert(!passphrase_ui_complete(&ui,&alias_old));
    assert(pp_owned_request.raw_len==256 && !memcmp(pp_owned_request.raw,alias_max,256));
    passphrase_ui_clear(&ui);for(unsigned i=0;i<256;i++)assert(ui.raw[i]==0);
    puts("PASS B1 whole/interior alias, invalid preservation, 256/257 boundaries; reviewer validation and owned snapshot probes");
    assert(passphrase_ui_raw_len(&ui)==0 && passphrase_ui_snapshot_marker(&ui)==PASSPHRASE_UI_EMPTY);
    assert(strstr(passphrase_ui_status(&ui), "Empty"));

    assert(passphrase_ui_set_raw(&ui, (const uint8_t *)" ", 1)==PASSPHRASE_UI_OK);
    assert(passphrase_ui_snapshot_marker(&ui)==PASSPHRASE_UI_ACTIVE);
    assert(!memcmp(passphrase_ui_raw(&ui), " ", 1));
    assert(strstr(lv_label_get_text(ui.counter), "1 / 256 UTF-8 bytes"));
    assert(passphrase_ui_set_raw(&ui, (const uint8_t *)"  CaFe  ", 8)==PASSPHRASE_UI_OK);
    assert(!memcmp(passphrase_ui_raw(&ui), "  CaFe  ", 8));

    uint8_t boundary[257]; memset(boundary, 'A', sizeof boundary);
    assert(passphrase_ui_set_raw(&ui,boundary,255)==PASSPHRASE_UI_OK);
    assert(passphrase_ui_editor_append(&ui,'B')==PASSPHRASE_UI_OK);
    assert(passphrase_ui_raw_len(&ui)==256);
    assert(passphrase_ui_editor_append(&ui,'C')==PASSPHRASE_UI_ERR_LIMIT);
    assert(passphrase_ui_raw_len(&ui)==256 && passphrase_ui_raw(&ui)[255]=='B');
    assert(passphrase_ui_set_raw(&ui,boundary,257)==PASSPHRASE_UI_ERR_LIMIT);
    const uint8_t cafe[]={'C','a','f',0xc3,0xa9};
    assert(passphrase_ui_set_raw(&ui,cafe,sizeof cafe)==PASSPHRASE_UI_OK);
    assert(passphrase_ui_raw_len(&ui)==sizeof cafe);
    assert(passphrase_ui_editor_append(&ui,0xc3)==PASSPHRASE_UI_ERR_EDITOR_UNSUPPORTED);
    const uint8_t bad[]={0xc3,0x28};
    assert(passphrase_ui_set_raw(&ui,bad,sizeof bad)==PASSPHRASE_UI_ERR_UTF8);

    assert(passphrase_ui_is_masked(&ui));
    pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_REVEAL));
    assert(passphrase_ui_modal_open(&ui) && passphrase_ui_is_masked(&ui));
    assert(lv_obj_has_flag(passphrase_ui_root(&ui),LV_OBJ_FLAG_CLICKABLE)==false);
    pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_KEEP_MASKED));
    assert(!passphrase_ui_modal_open(&ui) && passphrase_ui_is_masked(&ui));
    assert(passphrase_ui_focus_target(&ui)==passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_REVEAL));
    pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_REVEAL));
    pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_REVEAL_NOW));
    assert(!passphrase_ui_is_masked(&ui));
    pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_REVEAL));
    assert(passphrase_ui_is_masked(&ui));

    pp_submits=0; pp_accept=false;
    assert(passphrase_ui_set_raw(&ui,(const uint8_t*)"Case X",6)==PASSPHRASE_UI_OK);
    pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_CONTINUE));
    assert(pp_submits==1 && !passphrase_ui_pending(&ui) && passphrase_ui_is_masked(&ui));
    assert(!memcmp(pp_owned_request.raw,"Case X",6) && pp_owned_request.raw_len==6);
    assert(pp_owned_request.marker==PASSPHRASE_UI_ACTIVE);
    assert(strstr(passphrase_ui_status(&ui),"unavailable"));

    pp_accept=true; pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_CONTINUE));
    assert(passphrase_ui_pending(&ui));
    passphrase_ui_completion_t done={.request_id=pp_owned_request.request_id,.generation=pp_owned_request.generation,.context=pp_owned_request.context,.marker=pp_owned_request.marker,.rc=0};
    strcpy(done.fingerprint,"deadbeef"); strcpy(done.address,"bc1q-public-fixture-only");
    passphrase_ui_completion_t stale=done; stale.request_id--;
    assert(!passphrase_ui_complete(&ui,&stale));
    assert(passphrase_ui_complete(&ui,&done));
    assert(!passphrase_ui_complete(&ui,&done));
    assert(passphrase_ui_result_visible(&ui) && passphrase_ui_result_marker(&ui)==PASSPHRASE_UI_ACTIVE);

    passphrase_ui_back(&ui);
    assert(!passphrase_ui_result_visible(&ui) && passphrase_ui_raw_len(&ui)==6 && passphrase_ui_is_masked(&ui));
    pp_click(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_CONTINUE));
    done.request_id=pp_owned_request.request_id; done.generation=pp_owned_request.generation; done.context=pp_owned_request.context; done.marker=pp_owned_request.marker; done.rc=-3;
    assert(passphrase_ui_complete(&ui,&done)); assert(!passphrase_ui_result_visible(&ui)); assert(strstr(passphrase_ui_status(&ui),"failed"));
    passphrase_ui_back(&ui); assert(passphrase_ui_raw_len(&ui)==6);

    passphrase_ui_set_context(&ui,pp_context(1,8,12)); assert(passphrase_ui_raw_len(&ui)==0);
    passphrase_ui_set_raw(&ui,(const uint8_t*)"x",1); passphrase_ui_set_context(&ui,pp_context(1,9,12)); assert(passphrase_ui_raw_len(&ui)==0);
    passphrase_ui_set_raw(&ui,(const uint8_t*)"x",1); passphrase_ui_set_context(&ui,pp_context(1,9,15)); assert(passphrase_ui_raw_len(&ui)==0);
    passphrase_ui_set_raw(&ui,(const uint8_t*)"x",1); passphrase_ui_clear(&ui); assert(passphrase_ui_raw_len(&ui)==0 && passphrase_ui_is_masked(&ui));
    passphrase_ui_set_raw(&ui,(const uint8_t*)"x",1); passphrase_ui_cancel(&ui); assert(passphrase_ui_raw_len(&ui)==0);

    for(int i=0;i<PASSPHRASE_UI_CONTROL_COUNT;i++) pp_assert_target(passphrase_ui_control(&ui,(passphrase_ui_control_t)i));
    passphrase_ui_open_keyboard(&ui); assert(passphrase_ui_keyboard_open(&ui));
    for(unsigned i=0;i<passphrase_ui_keyboard_key_count(&ui);i++) pp_assert_target(passphrase_ui_keyboard_key(&ui,i));
    lv_obj_update_layout(lv_screen_active());
    lv_area_t a; lv_obj_get_coords(passphrase_ui_control(&ui,PASSPHRASE_UI_CONTROL_CONTINUE),&a); assert(a.y1>=0 && a.y2<800);
    assert(passphrase_ui_focus_target(&ui));
    unsigned defects=0; lv_area_t field, key, other, clip;
    lv_obj_get_coords(ui.field,&field); lv_obj_get_coords(ui.viewport,&clip);
    if(field.y1<clip.y1 || field.y2>clip.y2) { puts("RED editor clipped by viewport"); defects++; }
    for(unsigned i=0;i<PASSPHRASE_UI_KEY_MAX;i++) {
        lv_obj_get_coords(ui.keys[i],&key);
        for(unsigned j=i+1;j<PASSPHRASE_UI_KEY_MAX;j++) {
            lv_obj_get_coords(ui.keys[j],&other);
            if(key.x1<=other.x2 && other.x1<=key.x2 && key.y1<=other.y2 && other.y1<=key.y2) { puts("RED overlapping key rectangles"); defects++; }
        }
    }
    bool chars[128]={0}; chars[' ']=true;
    for(unsigned page=0;page<4;page++) {
        for(unsigned i=0;i<PASSPHRASE_UI_KEY_MAX;i++) {
            const char *label=lv_label_get_text(lv_obj_get_child(ui.keys[i],0));
            if(strlen(label)==1) chars[(unsigned char)label[0]]=true;
        }
        pp_click(ui.keys[PASSPHRASE_UI_KEY_MAX-1]);
    }
    for(unsigned c=32;c<127;c++) if(!chars[c]) { printf("RED missing ASCII %u\n",c); defects++; }
    fflush(stdout); assert(defects==0);
    pp_pointer=lv_indev_create();lv_indev_set_type(pp_pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(pp_pointer,pp_read);
    passphrase_ui_clear(&ui);
    for(unsigned page=0;page<4;page++){
        lv_obj_update_layout(ui.root);
        unsigned visible=0;for(unsigned k=0;k<32;k++)if(!lv_obj_has_flag(ui.keys[k],LV_OBJ_FLAG_HIDDEN))visible++;
        assert(lv_obj_get_height(ui.keyboard)<=(int)(((visible+7)/8)*48+56));
        pp_layout(&ui);size_t before=ui.raw_len;char expected=lv_label_get_text(lv_obj_get_child(ui.keys[0],0))[0];
        pp_touch(ui.keys[0]);assert(ui.raw_len==before+1 && ui.raw[before]==expected);
        pp_touch(ui.keys[34]);
    }
    assert(!memcmp(ui.raw,"aA0[",4));pp_touch(ui.keys[32]);assert(ui.raw[4]==' ');pp_touch(ui.keys[33]);assert(ui.raw_len==4);
    pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_REVEAL]);assert(ui.modal_open&&ui.masked);
    assert(lv_group_get_obj_count(ui.group)==2 && lv_group_get_focused(ui.group)==ui.controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]);
    lv_group_focus_next(ui.group);assert(lv_group_get_focused(ui.group)==ui.controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW]);lv_group_focus_next(ui.group);assert(lv_group_get_focused(ui.group)==ui.controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]);
    size_t held=ui.raw_len;pp_touch(ui.keys[0]);assert(ui.raw_len==held);
    lv_obj_send_event(ui.keys[0],LV_EVENT_CLICKED,NULL);assert(ui.raw_len==held);
    pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]);assert(!ui.modal_open&&ui.masked);
    lv_obj_send_event(ui.controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW],LV_EVENT_CLICKED,NULL);assert(ui.masked);
    pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_REVEAL]);pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW]);assert(!ui.masked);
    assert(!passphrase_ui_control(&ui,(passphrase_ui_control_t)-1));assert(!passphrase_ui_control(&ui,(passphrase_ui_control_t)99));
    pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_CONTINUE]);assert(ui.pending&&ui.masked);
    pp_touch(ui.keys[0]);assert(ui.raw_len==held&&ui.pending);assert(passphrase_ui_editor_append(&ui,'x')==PASSPHRASE_UI_ERR_BUSY);
    done=(passphrase_ui_completion_t){.request_id=pp_owned_request.request_id,.generation=pp_owned_request.generation,.context=pp_owned_request.context,.marker=pp_owned_request.marker};
    assert(!passphrase_ui_complete(&ui,NULL));stale=done;stale.generation++;assert(!passphrase_ui_complete(&ui,&stale));stale=done;stale.context.derivation_context++;assert(!passphrase_ui_complete(&ui,&stale));stale=done;stale.marker=(passphrase_ui_marker_t)9;assert(!passphrase_ui_complete(&ui,&stale));
    memset(done.fingerprint,'x',sizeof done.fingerprint);assert(passphrase_ui_complete(&ui,&done)&&!ui.result_visible);
    pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_CONTINUE]);done.request_id=pp_owned_request.request_id;done.generation=pp_owned_request.generation;memset(done.fingerprint,0,sizeof done.fingerprint);memset(done.address,'x',sizeof done.address);assert(passphrase_ui_complete(&ui,&done)&&!ui.result_visible);
    pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_CONTINUE]);done.request_id=pp_owned_request.request_id;done.generation=pp_owned_request.generation;
    passphrase_ui_context_t same=ui.context;passphrase_ui_set_context(&ui,same);assert(ui.pending&&ui.raw_len==held);
    same.derivation_context++;passphrase_ui_set_context(&ui,same);assert(!ui.pending&&ui.raw_len==0&&ui.masked);assert(!passphrase_ui_complete(&ui,&done));
    pp_touch(ui.controls[PASSPHRASE_UI_CONTROL_REVEAL]);same.source_revision++;passphrase_ui_set_context(&ui,same);assert(!ui.modal_open&&ui.masked);
    lv_obj_add_flag(ui.keyboard,LV_OBJ_FLAG_HIDDEN);held=ui.raw_len;lv_obj_send_event(ui.keys[0],LV_EVENT_CLICKED,NULL);assert(ui.raw_len==held);lv_obj_remove_flag(ui.keyboard,LV_OBJ_FLAG_HIDDEN);
    pp_layout(&ui);lv_indev_delete(pp_pointer);
    puts("PASS pointer: four ASCII pages, Space/Delete, reveal/cancel/confirm; modal focus trap; pending/hidden guards; invalid enum; malformed/stale completions; context interleavings");
    puts("PASS all 95 printable ASCII coverage; unobstructed ancestor bounds and pairwise visible-control/label rectangles");
    puts("PASS passphrase native LVGL behavior/state/geometry (public callback snapshot stub, NOT crypto)");
}
