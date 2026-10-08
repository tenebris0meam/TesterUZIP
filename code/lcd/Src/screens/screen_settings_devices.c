#include "screens/screen_settings_devices.h"
#include "cursor.h"
#include "strings_def.h"
#include "model.h"

void screen_settingsdevices_on_enter (void) {
    cursor_init(&options_strg[0], 
                params_get_devices_in_group(&params, test_config.group_id)->count,
                test_config.device_id);
    return;
}

void screen_settingsdevices_on_exit (void) {
    return;
}

void screen_settingsdevices_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, DEVICES_CYRILIC, NameList_Devices);
    return;
}

ScreenID screen_settingsdevices_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenSettingsDevices;
        case InputEvent_ButtonPressed_Cancel: return ScreenSettingsGroups;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenSettingsDevices;
        case InputEvent_ButtonPressed_Enter: {
            const Params_Range_TypeDef *devices_in_group = 
                params_get_devices_in_group(&params, test_config.group_id);
            model_select_device(devices_in_group->begin_idx + cursor_get_current_selected_index());
            return ScreenMain;
        }
        default: return ScreenSettingsDevices;
    }
}