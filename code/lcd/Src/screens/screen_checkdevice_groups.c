#include "screens/screen_settings_groups.h"
#include "cursor.h"
#include "strings_def.h"
#include "model.h"

void screen_settingsgroups_on_enter (void) {
    cursor_init(&options_strg[0], 
                params_get_groups_in_seria(&params, test_config.series_id)->count,
                test_config.group_id);
    return;
}

void screen_settingsgroups_on_exit (void) {
    return;
}

void screen_settingsgroups_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, GROUPS_CYRILIC, NameList_Groups);
    return;
}

ScreenID screen_settingsgroups_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenSettingsGroups;
        case InputEvent_ButtonPressed_Cancel: return ScreenSettingsSeries;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenSettingsGroups;
        case InputEvent_ButtonPressed_Enter: {
            const Params_Range_TypeDef *groups_in_seria = 
                params_get_groups_in_seria(&params, test_config.series_id);
            test_config.group_id = groups_in_seria->begin_idx + cursor_get_current_selected_index();
            return ScreenSettingsDevices;
        }
        default: return ScreenSettingsGroups;
    }
}