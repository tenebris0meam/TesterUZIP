#ifdef TESTER_MODES
#include "screens/screen_modes.h"
#include "proccessor.h"
#include "resolver.h"
#include "screens.h"
#include "cursor.h"
#include "model.h"
#include "strings_def.h"
#include "framebuffer.h"

#include <string.h>

void screen_modes_on_enter (void) {
    uint8_t item_count = Modes_Count;
    uint8_t item_selected = test_config.mode_id;
    bool is_item_count_bigger_workspace = (item_count > (workspace_height_in_chars - 2));
    bool is_items_fit_in_workspace = (item_selected + workspace_height_in_chars - 2) <= item_count;
    uint8_t item_first = (!is_item_count_bigger_workspace) ? 
                            0 : 
                            ((is_items_fit_in_workspace) ? 
                                item_selected : 
                                item_count - (workspace_height_in_chars - 2));
    cursor_init(&options_strg[0], 
                item_count, 
                item_selected, 
                item_first);
}

void screen_modes_render (void) {
    uint8_t cur_x = workspace_start_x;
    uint8_t cur_y = workspace_start_y;
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    const char *str_modes = MODES_CYRILIC;
    screen_get_center_from_string(&cur_x, strlen(str_modes));
    fb_draw_string_xy(cur_x, cur_y, str_modes);

    strbuf_clear(&sb);
    strbuf_str(&sb, TO_TEST_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, DEVICES_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, options_strg[0].y, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, VOLTMETER_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, options_strg[1].y, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, OMMETER_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, options_strg[2].y, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, GENERATOR_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, options_strg[3].y, strbuf_cstr(&sb));

    cursor_draw_highlight();
}

void screen_modes_on_exit (void) {
    return;
}

ScreenID screen_modes_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenModes;
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenModes;
        case InputEvent_ButtonPressed_Enter: 
            model_change_mode((Modes_TypeDef)cursor_get_current_selected_index()); 
            switch (test_config.mode_id) {
                case Voltmeter:
                case Ommeter:
                    proccessor_raise_event(EnterAccepted);
                case CheckDevice: 
                case VoltageGenerator: 
                default: 
                    return ScreenMain;
            }
        default: return ScreenModes;
    }
}
#endif