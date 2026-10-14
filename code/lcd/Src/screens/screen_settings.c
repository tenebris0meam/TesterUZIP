#include "screens/screen_settings.h"
#include "cursor.h"
#include "resolver.h"
#include "screens.h"
#include "strings_def.h"
#include "framebuffer.h"
#include "strbuf.h"
#include "model.h"

#include <string.h>


void screen_settings_on_enter (void) {
    switch (test_config.mode_id) {
        case CheckDevice: 
        case VoltageGenerator: 
            cursor_init(&options_strg[0], 2, 0);
            break;

        case Voltmeter: 
        case Ommeter: 
            cursor_init(&options_strg[1], 1, 0);
            break;

        default: break;
    }
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

    if (test_config.mode_id == CheckDevice) {
        strbuf_clear(&sb);
        strbuf_str(&sb, CHOSING_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, DEVICES_CYRILIC);
        screen_get_center_from_string(&cur_x, sb.len);
        fb_draw_string_xy(cur_x, options_strg[0].y, strbuf_cstr(&sb));
    } else if (test_config.mode_id == VoltageGenerator) {
        strbuf_clear(&sb);
        strbuf_str(&sb, SETTINGS_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, GENERATORA_CYRILIC);
        screen_get_center_from_string(&cur_x, sb.len);
        fb_draw_string_xy(cur_x, options_strg[0].y, strbuf_cstr(&sb));
    }

    strbuf_clear(&sb);
    strbuf_str(&sb, CHOSING_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, MODE_CYRILIC);
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
            switch (test_config.mode_id) {
                case CheckDevice: 
                    switch (cur_option) {
                        case 0: return ScreenSettingsSeries;
                        case 1: return ScreenSettingsModes;
                        default: return ScreenSettings;
                    }
                case VoltageGenerator: 
                    switch (cur_option) {
                        case 0: return ScreenGeneratorSetup;
                        case 1: return ScreenSettingsModes;
                        default: return ScreenSettings;
                    }
                case Voltmeter: 
                case Ommeter:
                    if (cur_option == 0) {
                        return ScreenSettingsModes;
                    } else {
                        return ScreenSettings;
                    }
                default: return ScreenSettings;
            }
        }
        default: return ScreenSettings;
    }
}