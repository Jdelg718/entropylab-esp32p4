static void assert_seed_button_caption_contained(lv_obj_t *button, const char *expected){
    lv_obj_t *caption=lv_obj_get_child(button,0);
    lv_area_t inner,content;
    assert(caption&&!strcmp(lv_label_get_text(caption),expected));
    lv_obj_get_coords(caption,&inner);
    lv_obj_get_content_coords(button,&content);
    fprintf(stderr,"seed button caption '%s' label=%d,%d-%d,%d content=%d,%d-%d,%d\n",
            expected,inner.x1,inner.y1,inner.x2,inner.y2,
            content.x1,content.y1,content.x2,content.y2);
    assert(inner.x1>=content.x1&&inner.x2<=content.x2);
    assert(inner.y1>=content.y1&&inner.y2<=content.y2);
}
static void seed_panel_event_test(void){
    lv_obj_clean(lv_screen_active());
    gui_create(request);
    gui_cards_bases_configure(gui08_passphrase_submit_probe,gui08_passphrase_cancel_probe);
    gui08_passphrase_submits=gui08_passphrase_cancels=0;
    memset(&gui08_passphrase_owned,0,sizeof gui08_passphrase_owned);

    click("Seed");
    assert(seed_native_visible(&seed_ui));
    assert(!gui08_native_visible(&gui08_ui));
    assert(find(lv_screen_active(),"Word numbers"));
    assert(find(lv_screen_active(),"Using 1-2048"));
    lv_obj_update_layout(lv_screen_active());
    assert_seed_button_caption_contained(seed_ui.delete_key,"Delete");

    click("0-2047");
    for(unsigned word=0;word<11;word++){
        click("0");
        click("Next word");
    }
    assert(seed_ui.committed==11);
    assert(seed_ui.candidate_count==128);
    assert(find(lv_screen_active(),"Choose the 12th word / 128 valid"));
    gui08_source_capture("seed-final-picker.ppm");

    click("about");
    assert(seed_native_ready(&seed_ui));
    assert(find(lv_screen_active(),"Checksum valid / ready to derive"));
    gui08_source_capture("seed-final-picked.ppm");

    click("Continue to Passphrase");
    assert(gui08_passphrase_submits==0);
    assert(pi_ui.live&&!lv_obj_has_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN));
    lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
    assert(gui08_passphrase_submits==1);
    assert(gui08_passphrase_owned.method==GUI08_SEED);
    assert(gui08_passphrase_owned.seed_operation==SH_NUMBERS_TO_WORDS);
    assert(gui08_passphrase_owned.base==0);
    assert(!strcmp(gui08_passphrase_owned.transcript,"0 0 0 0 0 0 0 0 0 0 0 3"));
    assert(gui08_passphrase_owned.passphrase_marker==GUI08_PASSPHRASE_EMPTY);
    assert(seed_ui.editor.pending_id==gui08_passphrase_owned.request_id);
    click("Cancel");
    assert(gui08_passphrase_cancels==1&&!seed_ui.editor.pending_id);

    assert(seed_ui.clear&&!lv_obj_has_state(seed_ui.clear,LV_STATE_DISABLED));
    lv_obj_send_event(seed_ui.clear,LV_EVENT_CLICKED,NULL);
    assert(!seed_native_ready(&seed_ui)&&seed_ui.committed==0);

    const unsigned counts[] = {12, 15, 18, 21, 24};
    const unsigned candidates[] = {128, 64, 32, 16, 8};
    for(unsigned base=0;base<2;base++){
        if(seed_ui.editor.base!=base) lv_obj_send_event(seed_ui.range,LV_EVENT_CLICKED,NULL);
        for(unsigned at=0;at<5;at++){
            lv_obj_send_event(seed_ui.word_buttons[at],LV_EVENT_CLICKED,NULL);
            assert(seed_ui.editor.words==counts[at]&&seed_ui.editor.base==base);
            for(unsigned word=0;word+1<counts[at];word++){
                lv_obj_send_event(seed_ui.digits[base],LV_EVENT_CLICKED,NULL);
                lv_obj_send_event(seed_ui.next_word,LV_EVENT_CLICKED,NULL);
            }
            assert(seed_ui.committed+1==counts[at]);
            assert(seed_ui.candidate_count==candidates[at]);
            lv_obj_send_event(seed_ui.candidate_buttons[0],LV_EVENT_CLICKED,NULL);
            assert(seed_native_ready(&seed_ui)&&seed_ui.committed==counts[at]);
            assert(seed_ui.editor.transcript[0]==(base?'1':'0'));
            lv_obj_send_event(seed_ui.clear,LV_EVENT_CLICKED,NULL);
            assert(!seed_native_ready(&seed_ui)&&seed_ui.committed==0);
        }
    }
    puts("PASS Seed native events: distinct route, both numbering ranges, all approved counts, valid last-word picker, copied request, exactly-once confirmation");
    gui_cards_bases_configure(NULL,NULL);
    gui_passphrase_dispose();
}
