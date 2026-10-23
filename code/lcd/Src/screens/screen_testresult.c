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
    cursor_init(&options_strg[3], 
                model_get_total_tests(), 
                screen_testdata_get_choosed_idx());
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
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    strbuf_u32(&sb, test_runtime.passed_cnt, 3);
    strbuf_char(&sb, '/');
    strbuf_u32(&sb, model_get_total_tests(), 3);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, PASSED_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    strbuf_clear(&sb);
    cur_y += cgxyz.height_px;

    uint8_t total = model_get_total_tests();
    uint8_t first = cursor_get_window_first();
    uint8_t test_seen = 0;
    uint8_t row = 0;

    for (uint8_t pos = 0; (pos < total) && (row < options_amount); pos++) {
        if (test_seen < first) {
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
            strbuf_char(&sb, ' ');
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
        char char_type;
        switch (pos_type) {
            case TransientVoltageSupressor: char_type = 'D'; break;
            case Varistor: char_type = 'B'; break;
            case GasDischargeTube: char_type = 'P'; break;
            default: char_type = '@'; break;
        }
        strbuf_char(&sb, char_type);
        screen_get_center_from_string(&cur_x, sb.len);
        fb_draw_string_xy(cur_x, 
                          options_strg[row].y, 
                          strbuf_cstr(&sb));
        strbuf_clear(&sb);
        test_seen++;
        row++;
    }
    if (is_cursor_init) {
        cursor_draw_highlight_row(cursor_get_current_selected_index() - first);
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