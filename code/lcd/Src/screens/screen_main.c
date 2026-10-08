#include "screens/screen_main.h"
#include "cursor.h"
#include "screens.h"
#include "strbuf.h"
#include "framebuffer.h"
#include "strings_def.h"
#include "fonts.h"
#include "model.h"

#include <string.h>


void screen_main_on_enter (void) {
    cursor_init(&options_strg[1], 2, 0);
    return;
}

void screen_main_render (void) {
    uint8_t cur_x = 0;
    StrBuf_TypeDef sb;

    strbuf_init(&sb, linebuffer, linebuffer_size);
    screen_append_device_name(&sb);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, workspace_start_y, strbuf_cstr(&sb));

    const char *str_interface = screen_interface_name(test_config.interface_id);
    screen_get_center_from_string(&cur_x, strlen(str_interface));
    fb_draw_string_xy(cur_x, 
                      workspace_start_y + cgxyz.height_px, 
                      str_interface);

    screen_get_center_from_string(&cur_x, strlen(TO_TEST_CYRILIC));
    fb_draw_string_xy(cur_x, options_strg[1].y, TO_TEST_CYRILIC);

    screen_get_center_from_string(&cur_x, strlen(SETTINGS_CYRILIC));
    fb_draw_string_xy(cur_x, options_strg[2].y, SETTINGS_CYRILIC);

    cursor_draw_highlight();
    return;
}

ScreenID screen_main_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenMain;
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenMain;
        case InputEvent_ButtonPressed_Enter: {
            uint8_t cur_option = cursor_get_current_selected_index();
            switch (cur_option) {
                case 0: return ScreenTesting;
                case 1: return ScreenSettings;
                default: return ScreenMain;
            }
        }
        default: return ScreenMain;
    }
}

void screen_main_on_exit (void) {
    return;
}