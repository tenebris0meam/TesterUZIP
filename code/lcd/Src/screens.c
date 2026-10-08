#include "screens.h"
#include "fonts.h"
#include "framebuffer.h"
#include "cursor.h"
#include "params.h"
#include "model.h"
#include "strings_def.h"
#include "strbuf.h"
#include "screens/screen_hello.h"
#include "screens/screen_main.h"
#include "screens/screen_testing.h"
#include "screens/screen_runningtest.h"
#include "screens/screen_result.h"
#include "screens/screen_checkfailed.h"
#include "screens/screen_settings.h"
#include "screens/screen_settings_interface.h"
#include "screens/screen_settings_series.h"
#include "screens/screen_settings_groups.h"
#include "screens/screen_settings_devices.h"
#include "screens/screen_hardware_fail.h"

#include <stdint.h>

const Screen_TypeDef screens[ScreenCount] = {
    [ScreenHello] = {
        .on_enter = screen_hello_on_enter,
        .on_exit = screen_hello_on_exit,
        .render = screen_hello_render, 
        .handle_event = screen_hello_handle_event
    },
    [ScreenMain] = {
        .on_enter = screen_main_on_enter,
        .on_exit = screen_main_on_exit,
        .render = screen_main_render,
        .handle_event = screen_main_handle_event
    },
    [ScreenTesting] = {
        .on_enter = screen_testing_on_enter,
        .on_exit = screen_testing_on_exit,
        .render = screen_testing_render,
        .handle_event = screen_testing_handle_event
    },
    [ScreenRunningTest] = {
        .on_enter = screen_runningtest_on_enter,
        .on_exit = screen_runningtest_on_exit,
        .render = screen_runningtest_render,
        .handle_event = screen_runningtest_handle_event
    }, 
    [ScreenResult] = {
        .on_enter = screen_result_on_enter,
        .on_exit = screen_result_on_exit,
        .render = screen_result_render, 
        .handle_event = screen_result_handle_event
    },
    [ScreenCheckFailed] = {
        .on_enter = screen_checkfailed_on_enter,
        .on_exit = screen_checkfailed_on_exit,
        .render = screen_checkfailed_render,
        .handle_event = screen_checkfailed_handle_event
    },
    [ScreenSettings] = {
        .on_enter = screen_settings_on_enter,
        .on_exit = screen_settings_on_exit,
        .render = screen_settings_render,
        .handle_event = screen_settings_handle_event
    },
    [ScreenSettingsInterface] = {
        .on_enter = screen_settingsinterface_on_enter,
        .on_exit = screen_settingsinterface_on_exit,
        .render = screen_settingsinterface_render,
        .handle_event = screen_settingsinterface_handle_event
    }, 
    [ScreenSettingsSeries] = {
        .on_enter = screen_settingsseries_on_enter,
        .on_exit = screen_settingsseries_on_exit,
        .render = screen_settingsseries_render,
        .handle_event = screen_settingsseries_handle_event
    },
    [ScreenSettingsGroups] = {
        .on_enter = screen_settingsgroups_on_enter,
        .on_exit = screen_settingsgroups_on_exit,
        .render = screen_settingsgroups_render,
        .handle_event = screen_settingsgroups_handle_event
    },
    [ScreenSettingsDevices] = {
        .on_enter = screen_settingsdevices_on_enter,
        .on_exit = screen_settingsdevices_on_exit,
        .render = screen_settingsdevices_render,
        .handle_event = screen_settingsdevices_handle_event
    }, 
    [ScreenHardwareFail] = {
        .on_enter = screen_hardwarefail_on_enter, 
        .on_exit = screen_hardwarefail_on_exit, 
        .render = screen_hardwarefail_render, 
        .handle_event = screen_hardwarefail_handle_event
    }
};



/* ------------------------------------------------------------------------ */
/*                          Вспомогательные средства                         */
/* ------------------------------------------------------------------------ */

char *linebuffer = 0;
uint8_t linebuffer_size = 0;

uint8_t workspace_start_x = 0;
uint8_t workspace_start_y = 0;
uint8_t workspace_width_in_px = 0;
uint8_t workspace_height_in_px = 0;
uint8_t workspace_width_in_chars = 0;
uint8_t workspace_height_in_chars = 0;

/**
 * @brief Название активного интерфейса.
 * @details Возвращает указатель на литерал во Flash - копировать его в буфер
 *          на стеке не нужно.
 */
const char *screen_interface_name (Param_Connector interface_id) {
    switch (interface_id) {
        case Probe: return PROBES_CYRILIC;
        case RJ45: return RJ45_CYRILIC;
        case K1: return K1_CYRILIC;
        default: return NOT_SUPPORTED_CYRILIC;
    }
}

/**
 * @brief Дописать имя из конфигурации.
 * @details Имена во Flash не завершены нулём, поэтому копируем по длине.
 *          Отсутствующее имя (индекс за границей каталога) пропускаем
 */
void screen_append_mystring (StrBuf_TypeDef *sb, const MyString *name) {
    if (name == 0) {
        return;
    }
    strbuf_strn(sb, name->beg, MyString_GetSize(name));
    return;
}

/** @brief Дописать имя выбранного прибора. */
void screen_append_device_name (StrBuf_TypeDef *sb) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return;
    }
    screen_append_mystring(sb, params_get_device_name(&params, device->device_idx));
    return;
}

/** @brief Вычислить координату x начала отцентрированной строки заданной длины */
void screen_get_center_from_string (uint8_t *center_x_px, 
                                           uint8_t str_length) {
    if (str_length >= workspace_width_in_chars) {
        *center_x_px = workspace_start_x;
    } else {
        *center_x_px = (FB_WIDTH - str_length * cgxyz.width_px) / 2;
    }
    return;
}

/** @brief Дописать "(вход,выход)" для тестовой последовательности. */
void screen_append_pins (StrBuf_TypeDef *sb, 
                                const Params_TestSequence_TypeDef *sequence) {
    strbuf_char(sb, '(');
    strbuf_u32(sb, (sequence != 0) ? sequence->input_pin : 0, 0);
    strbuf_char(sb, ',');
    strbuf_u32(sb, (sequence != 0) ? sequence->output_pin : 0, 0);
    strbuf_char(sb, ')');
    return;
}

/** @brief Заголовок вида "Выбор устройства". */
void screen_draw_title (StrBuf_TypeDef *sb, 
                               const char *first_word, 
                               const char *second_word) {
    uint8_t cur_x;
    strbuf_clear(sb);
    strbuf_str(sb, first_word);
    if (second_word != 0) {
        strbuf_char(sb, ' ');
        strbuf_str(sb, second_word);
    }
    screen_get_center_from_string(&cur_x, sb->len);
    fb_draw_string_xy(cur_x, workspace_start_y, strbuf_cstr(sb));
    return;
}

uint8_t screen_name_count (NameListKind kind) {
    switch (kind) {
        case NameList_Series: return params_get_series_num(&params);
        case NameList_Groups: {
            const Params_Range_TypeDef *groups_is_seria = 
                params_get_groups_in_seria(&params, test_config.series_id);
            return groups_is_seria->count;
        } 
        default: {
            const Params_Range_TypeDef *devices_in_group = 
                params_get_devices_in_group(&params, test_config.group_id);
            return devices_in_group->count;
        }
    }
}

const MyString *screen_name_at (NameListKind kind, uint8_t idx) {
    switch (kind) {
        case NameList_Series: return params_get_seria_name(&params, idx);
        case NameList_Groups: {
            const Params_Range_TypeDef *groups_in_seria = 
                params_get_groups_in_seria(&params, test_config.series_id);
            return params_get_group_name(&params, groups_in_seria->begin_idx + idx);
        }
        default: {
            const Params_Range_TypeDef *devices_in_group = 
                params_get_devices_in_group(&params, test_config.group_id);
            return params_get_device_name(&params, devices_in_group->begin_idx + idx);
        }
    }
}

/**
 * @brief Отрисовать список имён с прокруткой.
 * @details Окно из CURSOR_VISIBLE_ROWS строк сдвигается за курсором, поэтому
 *          список любой длины показывается корректно. 
 */
void screen_render_name_list (const char *first_word, 
                                     const char *second_word, 
                                     NameListKind kind) {
    uint8_t cur_x;
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    screen_draw_title(&sb, first_word, second_word);

    uint8_t count = screen_name_count(kind);
    uint8_t first = cursor_get_window_first();

    for (uint8_t row = 0; row < options_amount; row++) {
        uint8_t idx = (uint8_t)(first + row);
        if (idx >= count) {
            break;
        }
        strbuf_clear(&sb);
        screen_append_mystring(&sb, screen_name_at(kind, idx));
        screen_get_center_from_string(&cur_x, sb.len);
        fb_draw_string_xy(cur_x, 
                          options_strg[row].y, 
                          strbuf_cstr(&sb));
    }

    cursor_draw_highlight_row((uint8_t)(cursor_get_current_selected_index() - first));
    return;
}