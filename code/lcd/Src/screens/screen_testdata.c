#include "screens/screen_testdata.h"
#include "cursor.h"
#include "resolver.h"
#include "screens.h"
#include "strings_def.h"
#include "framebuffer.h"
#include "model.h"

#include <string.h>


void screen_testdata_on_enter (void) {
    cursor_init(&options_strg[0], test_results.failed_cnt, 0);
    return;
}

void screen_testdata_on_exit (void) {
    return;
}

void screen_testdata_render (void) {
    uint8_t cur_x = workspace_start_x;
    uint8_t cur_y = workspace_start_y;
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    const char *str_check = CHECK_LINES_CYRILIC;
    screen_get_center_from_string(&cur_x, strlen(str_check));
    fb_draw_string_xy(cur_x, cur_y, str_check);

    uint8_t total = model_get_total_tests();
    uint8_t first = cursor_get_window_first();
    uint8_t failed_seen = 0;
    uint8_t row = 0;

    for (uint8_t pos = 0; (pos < total) && (row < options_amount); pos++) {
        if (!model_is_test_failed(pos)) {
            continue;
        }
        if (failed_seen < first) {
            failed_seen++;
            continue;
        }
        ResolveType pos_type;
        if (pos < test_results.current_device->num_seqs_type0) {
            pos_type = TransientVoltageSupressor;
        } else if (pos < test_results.current_device->num_seqs_type0 + 
                        test_results.current_device->num_seqs_type1) {
            pos_type = Varistor;
        } else if (pos < test_results.current_device->num_seqs_type0 + 
                        test_results.current_device->num_seqs_type1 + 
                        test_results.current_device->num_seqs_type2) {
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
        failed_seen++;
        row++;
    }

    cursor_draw_highlight_row((uint8_t)(cursor_get_current_selected_index() - first));
    return;
}

ScreenID screen_testdata_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenTestData;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenTestData;
        case InputEvent_ButtonPressed_Enter: return ScreenMain;
        case InputEvent_ButtonPressed_Cancel: 
        default: return ScreenTestData;
    }
}