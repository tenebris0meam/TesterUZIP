#include "screens/screen_settings_interface.h"
#include "cursor.h"
#include "strings_def.h"
#include "model.h"
#include "framebuffer.h"

#include <string.h>

void screen_settingsinterface_on_enter (void) {
    uint8_t selected = 0;
    switch (test_config.interface_id) {
        case RJ45: selected = 1; break;
        case K1: selected = 2; break;
        default: selected = 0; break;
    }
    cursor_init(&options_strg[0], 3, selected);
    return; 
}

void screen_settingsinterface_on_exit (void) {
    return;
}

void screen_settingsinterface_render (void) {
    uint8_t cur_x;
    char *str = INTERFACE_CYRILIC;
    screen_get_center_from_string(&cur_x, strlen(str));
    fb_draw_string_xy(cur_x, workspace_start_y, str);
    
    str = PROBES_CYRILIC;
    screen_get_center_from_string(&cur_x, strlen(str));
    fb_draw_string_xy(cur_x, options_strg[0].y, str);

    str = RJ45_CYRILIC;
    screen_get_center_from_string(&cur_x, strlen(str));
    fb_draw_string_xy(cur_x, options_strg[1].y, RJ45_CYRILIC);

    str = K1_CYRILIC;
    screen_get_center_from_string(&cur_x, strlen(str));
    fb_draw_string_xy(cur_x, options_strg[2].y, K1_CYRILIC);

    cursor_draw_highlight();
    return;
}

ScreenID screen_settingsinterface_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: 
            cursor_move(-1); 
            return ScreenSettingsInterface;

        case InputEvent_ButtonPressed_Cancel: return ScreenSettings;
        case InputEvent_ButtonPressed_Next: 
            cursor_move(+1); 
            return ScreenSettingsInterface;

        case InputEvent_ButtonPressed_Enter: 
            switch (cursor_get_current_selected_index()) {
                case 0: test_config.interface_id = Probe; break;
                case 1: test_config.interface_id = RJ45; break;
                case 2: test_config.interface_id = K1; break;
                default: break;
            }
            return ScreenMain;
        default: return ScreenSettingsInterface;
    }
}