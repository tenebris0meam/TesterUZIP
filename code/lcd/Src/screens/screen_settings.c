#include "screens/screen_settings.h"
#include "cursor.h"
#include "strings_def.h"
#include "framebuffer.h"
#include "strbuf.h"

#include <string.h>


void screen_settings_on_enter (void) {
    cursor_init(&options_strg[0], 2, 0);
    return;
}

void screen_settings_on_exit (void) {
    return;
}

void screen_settings_render (void) {
    uint8_t cur_x = workspace_start_x;
    uint8_t cur_y = workspace_start_y;
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    const char *str_settings = SETTINGS_CYRILIC;
    screen_get_center_from_string(&cur_x, strlen(str_settings));
    fb_draw_string_xy(cur_x, cur_y, str_settings);

    strbuf_clear(&sb);
    strbuf_str(&sb, CHOSING_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, DEVICES_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, options_strg[0].y, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, CHOSING_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, INTERFACE_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, options_strg[1].y, strbuf_cstr(&sb));

    cursor_draw_highlight();
    return;
}

ScreenID screen_settings_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenSettings;
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenSettings;
        case InputEvent_ButtonPressed_Enter: {
            uint8_t cur_option = cursor_get_current_selected_index();
            switch (cur_option) {
                case 0: return ScreenSettingsSeries;
                case 1: return ScreenSettingsInterface;
                default: return ScreenSettings;
            }
        }
        default: return ScreenSettings;
    }
}