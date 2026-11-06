#include "screens/screen_checkdevice_groups.h"
#include "cursor.h"
#include "params.h"
#include "screens.h"
#include "strings_def.h"
#include "model.h"

void screen_checkdevice_groups_on_enter (void) {
    const Params_Range_TypeDef *groups_in_seria = params_get_groups_in_seria(&params, test_config_dirty.series_id);
    uint8_t item_count = groups_in_seria->count;
    uint8_t item_selected = test_config_dirty.group_id - groups_in_seria->begin_idx;
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

void screen_checkdevice_groups_on_exit (void) {
    return;
}

void screen_checkdevice_groups_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, 
                            GROUPS_CYRILIC, 
                            NameList_Groups, 
                            &test_config_dirty);
    return;
}

ScreenID screen_checkdevice_groups_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenCheckdeviceGroups;
        case InputEvent_ButtonPressed_Cancel: return ScreenCheckdeviceSeries;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenCheckdeviceGroups;
        case InputEvent_ButtonPressed_Enter: {
            const Params_Range_TypeDef *groups_in_seria = 
                params_get_groups_in_seria(&params, test_config_dirty.series_id);
            uint8_t selected = groups_in_seria->begin_idx + cursor_get_current_selected_index();
            if (selected != test_config_dirty.group_id) {
                test_config_dirty.group_id = selected;
                test_config_dirty.device_id = 0;
            }
            return ScreenCheckdeviceDevices;
        }
        default: return ScreenCheckdeviceGroups;
    }
}