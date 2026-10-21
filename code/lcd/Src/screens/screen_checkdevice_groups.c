#include "screens/screen_checkdevice_groups.h"
#include "cursor.h"
#include "strings_def.h"
#include "model.h"

void screen_checkdevice_groups_on_enter (void) {
    cursor_init(&options_strg[0], 
                params_get_groups_in_seria(&params, test_config.series_id)->count,
                test_config.group_id);
    return;
}

void screen_checkdevice_groups_on_exit (void) {
    return;
}

void screen_checkdevice_groups_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, GROUPS_CYRILIC, NameList_Groups);
    return;
}

ScreenID screen_checkdevice_groups_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenCheckdeviceGroups;
        case InputEvent_ButtonPressed_Cancel: return ScreenCheckdeviceSeries;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenCheckdeviceGroups;
        case InputEvent_ButtonPressed_Enter: {
            const Params_Range_TypeDef *groups_in_seria = 
                params_get_groups_in_seria(&params, test_config.series_id);
            test_config.group_id = groups_in_seria->begin_idx + cursor_get_current_selected_index();
            return ScreenCheckdeviceDevices;
        }
        default: return ScreenCheckdeviceGroups;
    }
}