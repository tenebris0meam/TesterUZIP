#include "screens.h"
#include "LCDFont7x14.h"
#include "tachyon_logo.h"
#include "framebuffer.h"
#include "input_events.h"
#include "cursor.h"
#include "params.h"
#include "model.h"
#include "proccessor.h"
#include "strings_def.h"
#include "strbuf.h"

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

/**
 * @brief Длина строки экрана в знакоместах плюс место под '\0'.
 * @details Экран 128 px, знакоместо 7 px - помещается 18 символов.
 */
#define LINE_MAX (19)

/** @brief Строка заголовка экрана. */
#define ROW_TITLE_Y (6)

/** @brief Какой список имён показывает экран настроек. */
typedef enum {
    NameList_Series,
    NameList_Groups,
    NameList_Devices
} NameListKind;

/**
 * @brief Название активного интерфейса.
 * @details Возвращает указатель на литерал во Flash - копировать его в буфер
 *          на стеке не нужно.
 */
static const char *screen_interface_name (Param_Connector interface_id) {
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
static void screen_append_mystring (StrBuf_TypeDef *sb, const MyString *name) {
    if (name == 0) {
        return;
    }
    strbuf_strn(sb, name->beg, MyString_GetSize(name));
    return;
}

/** @brief Дописать имя выбранного прибора. */
static void screen_append_device_name (StrBuf_TypeDef *sb) {
    const Params_Device_TypeDef *device = test_results.current_device;
    if (device == 0) {
        return;
    }
    screen_append_mystring(sb, params_get_device_name(&params, device->device_idx));
    return;
}

/** @brief Дописать "(вход,выход)" для тестовой последовательности. */
static void screen_append_pins (StrBuf_TypeDef *sb, 
                                const Params_TestSequence_TypeDef *sequence) {
    strbuf_char(sb, '(');
    strbuf_u32(sb, (sequence != 0) ? sequence->input_pin : 0, 0);
    strbuf_char(sb, ',');
    strbuf_u32(sb, (sequence != 0) ? sequence->output_pin : 0, 0);
    strbuf_char(sb, ')');
    return;
}

/** @brief Заголовок вида "Выбор устройства". */
static void screen_draw_title (StrBuf_TypeDef *sb, 
                               const char *first_word, 
                               const char *second_word) {
    strbuf_clear(sb);
    strbuf_str(sb, first_word);
    if (second_word != 0) {
        strbuf_char(sb, ' ');
        strbuf_str(sb, second_word);
    }
    fb_draw_string_xy(1, ROW_TITLE_Y, strbuf_cstr(sb));
    return;
}

static uint8_t screen_name_count (NameListKind kind) {
    switch (kind) {
        case NameList_Series: return params_get_series_num(&params);
        case NameList_Groups: return params_get_groups_num(&params);
        default: return params_get_devices_num(&params);
    }
}

static const MyString *screen_name_at (NameListKind kind, uint8_t idx) {
    switch (kind) {
        case NameList_Series: return params_get_seria_name(&params, idx);
        case NameList_Groups: return params_get_group_name(&params, idx);
        default: return params_get_device_name(&params, idx);
    }
}

/**
 * @brief Отрисовать список имён с прокруткой.
 * @details Окно из CURSOR_VISIBLE_ROWS строк сдвигается за курсором, поэтому
 *          список любой длины показывается корректно. 
 */
static void screen_render_name_list (const char *first_word, 
                                     const char *second_word, 
                                     NameListKind kind) {
    char line[LINE_MAX];
    StrBuf_TypeDef sb;
    strbuf_init(&sb, line, sizeof(line));

    screen_draw_title(&sb, first_word, second_word);

    uint8_t count = screen_name_count(kind);
    uint8_t first = cursor_get_window_first();

    for (uint8_t row = 0; row < CURSOR_VISIBLE_ROWS; row++) {
        uint8_t idx = (uint8_t)(first + row);
        if (idx >= count) {
            break;
        }
        strbuf_clear(&sb);
        screen_append_mystring(&sb, screen_name_at(kind, idx));
        fb_draw_string_xy(options_in_list[row].x, 
                          options_in_list[row].y, 
                          strbuf_cstr(&sb));
    }

    cursor_draw_highlight_row((uint8_t)(cursor_get_current_selected_index() - first));
    return;
}



/* ------------------------------------------------------------------------ */
/*                                  on_enter                                 */
/* ------------------------------------------------------------------------ */

void screen_hello_on_enter (void) {
    return;
}

void screen_main_on_enter (void) {
    cursor_init(&options_in_list[1], 2, 0);
    return;
}

void screen_testing_on_enter (void) {
    return;
}

void screen_runningtest_on_enter (void) {
    /* Новый прогон по тому же прибору должен начинаться с чистых результатов */
    model_reset_results();
    return;
}

void screen_result_on_enter (void) {
    return;
}

void screen_checkfailed_on_enter (void) {
    cursor_init(&options_in_list[0], test_results.failed_cnt, 0);
    return;
}

void screen_settings_on_enter (void) {
    cursor_init(&options_in_list[0], 2, 0);
    return;
}

void screen_settingsinterface_on_enter (void) {
    uint8_t selected = 0;
    switch (test_config.interface_id) {
        case RJ45: selected = 1; break;
        case K1: selected = 2; break;
        default: selected = 0; break;
    }
    cursor_init(&options_in_list[0], 3, selected);
    return; 
}

void screen_settingsseries_on_enter (void) {
    cursor_init(&options_in_list[0], 
                params_get_series_num(&params), 
                test_config.series_id);
    return;
}

void screen_settingsgroups_on_enter (void) {
    cursor_init(&options_in_list[0], 
                params_get_groups_num(&params),
                test_config.group_id);
    return;
}

void screen_settingsdevices_on_enter (void) {
    cursor_init(&options_in_list[0], 
                params_get_devices_num(&params),
                test_config.device_id);
    return;
}

void screen_hardwarefail_on_enter (void) {
    return;
}



/* ------------------------------------------------------------------------ */
/*                                  on_exit                                  */
/* ------------------------------------------------------------------------ */

void screen_hello_on_exit (void) {
    return;
}

void screen_main_on_exit (void) {
    return;
}

void screen_testing_on_exit (void) {
    return;
}

void screen_runningtest_on_exit (void) {
    return;
}

void screen_result_on_exit (void) {
    return;
}

void screen_checkfailed_on_exit (void) {
    return;
}

void screen_settings_on_exit (void) {
    return;
}

void screen_settingsinterface_on_exit (void) {
    return;
}

void screen_settingsseries_on_exit (void) {
    return;
}

void screen_settingsgroups_on_exit (void) {
    return;
}

void screen_settingsdevices_on_exit (void) {
    return;
}

void screen_hardwarefail_on_exit (void) {
    return;
}



/* ------------------------------------------------------------------------ */
/*                                  render                                   */
/* ------------------------------------------------------------------------ */

void screen_hello_render (void) {
    fb_draw_bitmap((FB_WIDTH - LOGO_WIDTH) / 2, 
                   (FB_HEIGHT - LOGO_HEIGHT) / 2, 
                   LOGO_WIDTH, 
                   LOGO_HEIGHT, 
                   logo);
    return;
}

void screen_main_render (void) {
    char line[LINE_MAX];
    StrBuf_TypeDef sb;

    strbuf_init(&sb, line, sizeof(line));
    screen_append_device_name(&sb);
    fb_draw_string_xy(1, ROW_TITLE_Y, strbuf_cstr(&sb));

    fb_draw_string_xy(64, ROW_TITLE_Y, screen_interface_name(test_config.interface_id));
    fb_draw_string_xy(options_in_list[1].x, options_in_list[1].y, TO_TEST_CYRILIC);
    fb_draw_string_xy(options_in_list[2].x, options_in_list[2].y, SETTINGS_CYRILIC);
    cursor_draw_highlight();
    return;
}

void screen_testing_render (void) {
    char line[LINE_MAX];
    StrBuf_TypeDef sb;

    strbuf_init(&sb, line, sizeof(line));
    strbuf_str(&sb, TO_TEST_CYRILIC);
    strbuf_char(&sb, ' ');
    screen_append_device_name(&sb);
    fb_draw_string_xy(1, ROW_TITLE_Y, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, INTERFACE_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, screen_interface_name(test_config.interface_id));
    fb_draw_string_xy(1, 19, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, CONTINUE_CYRILIC);
    strbuf_char(&sb, '?');
    fb_draw_string_xy(1, 32, strbuf_cstr(&sb));
    return;
}

void screen_runningtest_render (void) {
    char line[LINE_MAX];
    StrBuf_TypeDef sb;

    strbuf_init(&sb, line, sizeof(line));
    strbuf_str(&sb, TO_TEST_CYRILIC);
    strbuf_char(&sb, ' ');
    screen_append_device_name(&sb);
    fb_draw_string_xy(1, ROW_TITLE_Y, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, INTERFACE_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, screen_interface_name(test_config.interface_id));
    fb_draw_string_xy(1, 19, strbuf_cstr(&sb));

    // Третья линия пропущена

    strbuf_clear(&sb);
    screen_append_pins(&sb, test_results.current_sequence);
    strbuf_str(&sb, "      ");
    strbuf_u32(&sb, test_results.passed_cnt, 3);
    strbuf_char(&sb, '/');
    strbuf_u32(&sb, model_get_total_tests(), 3);
    fb_draw_string_xy(1, 45, strbuf_cstr(&sb));
    return;
}

void screen_result_render (void) {
    // Пропускаем первую линию

    char line[LINE_MAX];
    StrBuf_TypeDef sb;

    strbuf_init(&sb, line, sizeof(line));
    strbuf_str(&sb, test_results.fail_was ? FAILED_CYRILIC : SUCCESS_CYRILIC);
    fb_draw_string_xy(1, 19, strbuf_cstr(&sb));

    // Пропускаем третью линию

    strbuf_clear(&sb);
    strbuf_u32(&sb, test_results.passed_cnt, 3);
    strbuf_char(&sb, '/');
    strbuf_u32(&sb, model_get_total_tests(), 3);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, PASSED_CYRILIC);
    fb_draw_string_xy(1, 45, strbuf_cstr(&sb));
    return;
}

void screen_checkfailed_render (void) {
    char line[LINE_MAX];
    StrBuf_TypeDef sb;
    strbuf_init(&sb, line, sizeof(line));

    fb_draw_string_xy(1, ROW_TITLE_Y, CHECK_LINES_CYRILIC);

    uint8_t total = model_get_total_tests();
    uint8_t first = cursor_get_window_first();
    uint8_t failed_seen = 0;
    uint8_t row = 0;

    for (uint8_t pos = 0; (pos < total) && (row < CURSOR_VISIBLE_ROWS); pos++) {
        if (!model_is_test_failed(pos)) {
            continue;
        }
        if (failed_seen < first) {
            failed_seen++;
            continue;
        }
        strbuf_clear(&sb);
        screen_append_pins(&sb, model_get_sequence_at(pos));
        fb_draw_string_xy(options_in_list[row].x, 
                          options_in_list[row].y, 
                          strbuf_cstr(&sb));
        failed_seen++;
        row++;
    }

    // Курсор не выделяем
    return;
}

void screen_settings_render (void) {
    char line[LINE_MAX];
    StrBuf_TypeDef sb;
    strbuf_init(&sb, line, sizeof(line));

    fb_draw_string_xy(1, ROW_TITLE_Y, SETTINGS_CYRILIC);

    strbuf_clear(&sb);
    strbuf_str(&sb, CHOSING_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, DEVICES_CYRILIC);
    fb_draw_string_xy(options_in_list[0].x, options_in_list[0].y, strbuf_cstr(&sb));

    strbuf_clear(&sb);
    strbuf_str(&sb, CHOSING_CYRILIC);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, INTERFACE_CYRILIC);
    fb_draw_string_xy(options_in_list[1].x, options_in_list[1].y, strbuf_cstr(&sb));

    cursor_draw_highlight();
    return;
}

void screen_settingsinterface_render (void) {
    fb_draw_string_xy(1, ROW_TITLE_Y, INTERFACE_CYRILIC);
    fb_draw_string_xy(options_in_list[0].x, options_in_list[0].y, PROBES_CYRILIC);
    fb_draw_string_xy(options_in_list[1].x, options_in_list[1].y, RJ45_CYRILIC);
    fb_draw_string_xy(options_in_list[2].x, options_in_list[2].y, K1_CYRILIC);
    cursor_draw_highlight();
    return;
}

void screen_settingsseries_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, SERIES_CYRILIC, NameList_Series);
    return;
}

void screen_settingsgroups_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, GROUPS_CYRILIC, NameList_Groups);
    return;
}

void screen_settingsdevices_render (void) {
    screen_render_name_list(CHOSING_CYRILIC, DEVICES_CYRILIC, NameList_Devices);
    return;
}

void screen_hardwarefail_render (void) {
    fb_set_font(JetBrains_Mono7x13_Latin, 
                LCDFONT_WIDTH, 
                LCDFONT_HEIGHT, 
                LCDFONT_FIRST_CHAR, 
                LCDFONT_LAST_CHAR);
    fb_draw_string_xy(1, 19, "Hardware Fail");
    fb_set_font(JetBrains_Mono7x13_Cyrilic, 
                LCDFONT_WIDTH, 
                LCDFONT_HEIGHT, 
                LCDFONT_FIRST_CHAR, 
                LCDFONT_LAST_CHAR);
    return;
}



/* ------------------------------------------------------------------------ */
/*                               handle_event                                */
/* ------------------------------------------------------------------------ */

ScreenID screen_hello_handle_event(InputEvent event) {
    switch (event) {
        case InputEvent_HelloScreenTimeout: return ScreenMain;
        default: return ScreenHello;
    }
}

ScreenID screen_main_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenMain;
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenMain;
        case InputEvent_ButtonPressed_Enter: {
            uint8_t cur_option = cursor_get_current_selected_index();
            switch (cur_option) {
                case 0: return ScreenTesting;
                case 1: return ScreenSettings;
                default: return ScreenMain;
            }
        }
        default: return ScreenMain;
    }
}

ScreenID screen_testing_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Enter: 
            proccessor_raise_event(EnterAccepted);
            return ScreenRunningTest;
        default: return ScreenTesting;
    }
}

ScreenID screen_runningtest_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_TestSequenceComplete: return ScreenResult;
        default: return ScreenRunningTest;
    }
}

ScreenID screen_result_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Enter: 
            if (test_results.fail_was) {
                return ScreenCheckFailed;
            } else {
                return ScreenMain;
            }
        default: return ScreenResult;
    }
}

ScreenID screen_checkfailed_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenCheckFailed;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenCheckFailed;
        case InputEvent_ButtonPressed_Enter: return ScreenMain;
        case InputEvent_ButtonPressed_Cancel: 
        default: return ScreenCheckFailed;
    }
}

ScreenID screen_settings_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenSettings;
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenSettings;
        case InputEvent_ButtonPressed_Enter: {
            uint8_t cur_option = cursor_get_current_selected_index();
            switch (cur_option) {
                case 0: return ScreenSettingsSeries;
                case 1: return ScreenSettingsInterface;
                default: return ScreenSettings;
            }
        }
        default: return ScreenSettings;
    }
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

ScreenID screen_settingsseries_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenSettingsSeries;
        case InputEvent_ButtonPressed_Cancel: return ScreenSettings;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenSettingsSeries;
        case InputEvent_ButtonPressed_Enter: 
            test_config.series_id = cursor_get_current_selected_index();
            return ScreenSettingsGroups;
        default: return ScreenSettingsSeries;
    }
}

ScreenID screen_settingsgroups_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenSettingsGroups;
        case InputEvent_ButtonPressed_Cancel: return ScreenSettingsSeries;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenSettingsGroups;
        case InputEvent_ButtonPressed_Enter: 
            test_config.group_id = cursor_get_current_selected_index();
            return ScreenSettingsDevices;
        default: return ScreenSettingsGroups;
    }
}

ScreenID screen_settingsdevices_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenSettingsDevices;
        case InputEvent_ButtonPressed_Cancel: return ScreenSettingsGroups;
        case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenSettingsDevices;
        case InputEvent_ButtonPressed_Enter: 
            model_select_device(cursor_get_current_selected_index());
            return ScreenMain;
        default: return ScreenSettingsDevices;
    }
}

ScreenID screen_hardwarefail_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Enter: 
            /* Оператор подтвердил, что видел аварию - процессор вернётся в Idle */
            proccessor_raise_event(EnterAccepted);
            return ScreenMain;
        default: return ScreenHardwareFail;
    }
}
