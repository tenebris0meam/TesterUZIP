#include "screens/screen_checkdevice_series.h"
#include "cursor.h"
#include "model.h"
#include "strings_def.h"
#include <string.h>

void screen_checkdevice_series_on_enter (void) {
    memcpy(&test_config_dirty, &test_config, sizeof(TestConfig_TypeDef));
    uint8_t item_count = params_get_series_num(&params);
    uint8_t item_selected = test_config_dirty.series_id;
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

void screen_checkdevice_series_on_exit (void) {
    return;
}

void screen_checkdevice_series_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, 
                            SERIES_CYRILIC, 
                            NameList_Series, 
                            &test_config_dirty);
    return;
}

ScreenID screen_checkdevice_series_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenCheckdeviceSeries;
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenCheckdeviceSeries;
        case InputEvent_ButtonPressed_Enter: {
            uint8_t selected = cursor_get_current_selected_index();
            if (selected != test_config_dirty.series_id) {
                test_config_dirty.series_id = selected;
                test_config_dirty.group_id = 0;
                test_config_dirty.device_id = 0;
            }
            return ScreenCheckdeviceGroups;
        }
        default: return ScreenCheckdeviceSeries;
    }
}