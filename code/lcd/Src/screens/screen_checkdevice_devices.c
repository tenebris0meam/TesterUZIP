#include "screens/screen_checkdevice_devices.h"
#include "cursor.h"
#include "params.h"
#include "screens.h"
#include "strings_def.h"
#include "model.h"
#include <string.h>

void screen_checkdevice_devices_on_enter (void) {
    const Params_Range_TypeDef *devices_in_group = params_get_devices_in_group(&params, test_config_dirty.group_id);
    uint8_t item_count = devices_in_group->count;
    uint8_t item_selected = test_config_dirty.device_id - devices_in_group->begin_idx;
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
    return;
}

void screen_checkdevice_devices_on_exit (void) {
    return;
}

void screen_checkdevice_devices_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, 
                            DEVICES_CYRILIC, 
                            NameList_Devices, 
                            &test_config_dirty);
    return;
}

ScreenID screen_checkdevice_devices_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenCheckdeviceDevices;
        case InputEvent_ButtonPressed_Cancel: return ScreenCheckdeviceGroups;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenCheckdeviceDevices;
        case InputEvent_ButtonPressed_Enter: {
            const Params_Range_TypeDef *devices_in_group = 
                params_get_devices_in_group(&params, test_config_dirty.group_id);
            uint8_t selected = devices_in_group->begin_idx + cursor_get_current_selected_index();
            if ((selected != test_config_dirty.device_id) || 
                (memcmp(&test_config_dirty, &test_config, sizeof(TestConfig_TypeDef)))) {
                model_select_device(selected);
            }
            return ScreenMain;
        }
        default: return ScreenCheckdeviceDevices;
    }
}