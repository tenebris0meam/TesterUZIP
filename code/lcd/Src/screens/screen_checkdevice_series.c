#include "screens/screen_checkdevice_series.h"
#include "cursor.h"
#include "model.h"
#include "strings_def.h"

void screen_checkdevice_series_on_enter (void) {
    cursor_init(&options_strg[0], 
                params_get_series_num(&params), 
                test_config.series_id);
    return;
}

void screen_checkdevice_series_on_exit (void) {
    return;
}

void screen_checkdevice_series_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, SERIES_CYRILIC, NameList_Series);
    return;
}

ScreenID screen_checkdevice_series_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenCheckdeviceSeries;
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenCheckdeviceSeries;
        case InputEvent_ButtonPressed_Enter: 
            test_config.series_id = cursor_get_current_selected_index();
            return ScreenCheckdeviceGroups;
        default: return ScreenCheckdeviceSeries;
    }
}