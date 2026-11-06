#include "screens/screen_testresult.h"
#include "input_events.h"
#include "screen_testdata.h"
#include "screens.h"
#include "strbuf.h"
#include "strings_def.h"
#include "fonts.h"
#include "framebuffer.h"
#include "model.h"
#include "cursor.h"

static bool is_cursor_init = false;

void screen_testresult_on_enter (void) {
    uint8_t item_count = model_get_total_tests();
    uint8_t item_selected = screen_testdata_get_choosed_idx();
    bool is_item_count_bigger_workspace = (item_count > (workspace_height_in_chars - 2));
    bool is_items_fit_in_workspace = (item_selected + workspace_height_in_chars - 2) <= item_count;
    uint8_t item_first = (!is_item_count_bigger_workspace) ? 
                            0 : 
                            ((is_items_fit_in_workspace) ? 
                                item_selected : 
                                item_count - (workspace_height_in_chars - 2));
    cursor_init(&options_strg[3], 
                item_count, 
                item_selected,
                item_first);
    return;
}

void screen_testresult_on_exit (void) {
    return;
}

void screen_testresult_render (void) {
    uint8_t cur_x = workspace_start_x;
    uint8_t cur_y = workspace_start_y;

    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);
    strbuf_str(&sb, test_runtime.fail_was ? FAILED_CYRILIC : SUCCESS_CYRILIC);
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    strbuf_str(&sb, PASSED_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_u32(&sb, test_runtime.passed_cnt, 1);
    strbuf_str(&sb, " hg ");
    strbuf_u32(&sb, model_get_total_tests(), 1);
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    uint8_t total = model_get_total_tests();
    uint8_t selected = cursor_get_current_selected_index();
    if (selected < cursor.window_first) {
        cursor.window_first = selected;
    }
    if ((selected - cursor.window_first) == options_amount) {
        cursor.window_first++;
    }

    uint8_t test_seen = 0;
    uint8_t row = 0;

    for (uint8_t pos = 0; (pos < total) && (row < options_amount); pos++) {
        if (test_seen < cursor.window_first) {
            test_seen++;
            continue;
        }

        cur_x = workspace_start_x;
        strbuf_u32(&sb, pos + 1, 1);
        strbuf_char(&sb, ' ');
        if (model_is_test_failed(pos)) {
            // рисуем крестик
            strbuf_char(&sb, '&');
        } else if (model_is_test_passed(pos)) {
            // рисуем галочку
            strbuf_char(&sb, '%');
        } else {
            // непредвиденная ошибка
            strbuf_char(&sb, '?');
        }
        fb_draw_string_xy(cur_x, options_strg[row].y, strbuf_cstr(&sb));
        strbuf_clear(&sb);

        ResolveType pos_type;
        if (pos < test_runtime.current_device->num_seqs_type0) {
            pos_type = TransientVoltageSupressor;
        } else if (pos < test_runtime.current_device->num_seqs_type0 + 
                        test_runtime.current_device->num_seqs_type1) {
            pos_type = Varistor;
        } else if (pos < test_runtime.current_device->num_seqs_type0 + 
                        test_runtime.current_device->num_seqs_type1 + 
                        test_runtime.current_device->num_seqs_type2) {
            pos_type = GasDischargeTube;
        }
        strbuf_clear(&sb);
        screen_append_pins(&sb, model_get_sequence_at(pos, pos_type));
        strbuf_char(&sb, ' ');
        switch (pos_type) {
            case TransientVoltageSupressor: strbuf_str(&sb, DIODE_CYRILIC); break;
            case Varistor: strbuf_str(&sb, VARIS_CYRILIC); break;
            case GasDischargeTube: strbuf_str(&sb, DISCH_CYRILIC); break;
            default: strbuf_char(&sb, '@'); break;
        }
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    options_strg[row].y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        test_seen++;
        row++;
    }
    if (is_cursor_init) {
        cursor_draw_highlight_row(selected - cursor.window_first);
    }
    return;
}

ScreenID screen_testresult_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Next: 
            if (is_cursor_init) {
                cursor_move(+1); 
            }
            return ScreenTestResult;

        case InputEvent_ButtonPressed_Previous: 
            if (is_cursor_init) {
                cursor_move(-1); 
            }
            return ScreenTestResult;

        case InputEvent_ButtonPressed_Enter:
            if (!is_cursor_init) {
                is_cursor_init = true;
                return ScreenTestResult;
            } else {
                screen_testdata_set_choosed_idx(cursor_get_current_selected_index());
                return ScreenTestData;
            }

        case InputEvent_ButtonPressed_Cancel: 
            if (is_cursor_init) {
                is_cursor_init = false;
                return ScreenTestResult;
            } else {
                screen_testdata_set_choosed_idx(0);
                return ScreenMain;
            }
            
        default: return ScreenTestResult;
    }
}