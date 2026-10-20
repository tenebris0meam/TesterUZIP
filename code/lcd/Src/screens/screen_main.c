#include "screens/screen_main.h"
#include "cursor.h"
#include "generation.h"
#include "proccessor.h"
#include "resolver.h"
#include "calc.h"
#include "screens.h"
#include "strbuf.h"
#include "framebuffer.h"
#include "strings_def.h"
#include "fonts.h"
#include "model.h"

#include <string.h>


void screen_main_on_enter (void) {
    switch (test_config.mode_id) {
        case CheckDevice: 
        case VoltageGenerator:
            cursor_init(&options_strg[1], 2, 0);
            break;

        case Voltmeter: 
        case Ommeter: 
            cursor_init(&options_strg[2], 1, 0);
            break;

        default: return;
    }
    return;
}

void screen_main_render (void) {
    uint8_t cur_x = 0;
    uint8_t cur_y = workspace_start_y;
    StrBuf_TypeDef sb;

    strbuf_init(&sb, linebuffer, linebuffer_size);
    switch (test_config.mode_id) {
        case CheckDevice: {
            screen_append_device_name(&sb);
            screen_get_center_from_string(&cur_x, sb.len);
            fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
            cur_y += cgxyz.height_px;
            strbuf_clear(&sb);

            const char *str_interface = screen_interface_name(test_results.current_device->connector_id);
            screen_get_center_from_string(&cur_x, strlen(str_interface));
            fb_draw_string_xy(cur_x, cur_y, str_interface);

            screen_get_center_from_string(&cur_x, strlen(TO_TEST_CYRILIC));
            fb_draw_string_xy(cur_x, options_strg[1].y, TO_TEST_CYRILIC);
            break;
        }

        case VoltageGenerator: {
            strbuf_str(&sb, GENERATOR_CYRILIC);
            screen_get_center_from_string(&cur_x, sb.len);
            fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
            cur_y += cgxyz.height_px;
            strbuf_clear(&sb);

            strbuf_u32(&sb, proccessor_get_manual_target_voltage(), 1);
            strbuf_str(&sb, " B");
            screen_get_center_from_string(&cur_x, sb.len);
            fb_draw_string_xy(cur_x, 
                            workspace_start_y + cgxyz.height_px, 
                            strbuf_cstr(&sb));

            screen_get_center_from_string(&cur_x, strlen(GENERATION_CYRILIC));
            fb_draw_string_xy(cur_x, options_strg[1].y, GENERATION_CYRILIC);
            break;
        }

        case Voltmeter: {
            strbuf_init(&sb, linebuffer, linebuffer_size);
            strbuf_str(&sb, VOLTMETER_CYRILIC);
            screen_get_center_from_string(&cur_x, sb.len);
            fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
            strbuf_clear(&sb);
            cur_y += cgxyz.height_px;

            int32_t voltage_uV = 0, current_nA = 0;
            calc_get_test_data(&voltage_uV, &current_nA);
            int32_t voltage_V = voltage_uV / 1000000L;
            int32_t voltage_mV = (voltage_uV % 1000000L) / 1000L;
            int32_t current_mA = current_nA / 1000000L;
            int32_t current_uA = (current_nA % 1000000L) / 1000L;
            strbuf_u32(&sb, voltage_V, 3U);
            strbuf_char(&sb, '.');
            strbuf_u32(&sb, voltage_mV, 3U);
            strbuf_str(&sb, " B   ");
            if (current_nA < 0) {
                strbuf_char(&sb, '-');
                strbuf_u32(&sb, (uint32_t)-current_mA, 1U);
                strbuf_char(&sb, '.');
                strbuf_u32(&sb, (uint32_t)-current_uA, 3U);
            } else {
                strbuf_u32(&sb, (uint32_t)current_mA, 1U);
                strbuf_char(&sb, '.');
                strbuf_u32(&sb, (uint32_t)current_uA, 3U);
            }
            strbuf_str(&sb, " l@");

            screen_get_center_from_string(&cur_x, (uint8_t)sb.len);
            fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
            proccessor_raise_event(DataOutputed);
            break;
        }
        case Ommeter: {
            strbuf_init(&sb, linebuffer, linebuffer_size);

            OmmeterStatus status = proccessor_ommeter_get_status();
            strbuf_str(&sb, OMMETER_CYRILIC);
            screen_get_center_from_string(&cur_x, sb.len);
            fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
            strbuf_clear(&sb);
            cur_y += cgxyz.height_px;

            switch (status) {
                case Ommeter_Invalid:
                    strbuf_str(&sb, "---");
                    screen_get_center_from_string(&cur_x, sb.len);
                    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
                    break;
                case Ommeter_Valid: {
                    int32_t Ohms = proccessor_ommeter_get_resistance_ohm();
                    int32_t MegaOhms = Ohms / 1000000L;
                    int32_t kiloOhms = Ohms / 1000L;
                    if (MegaOhms) {
                        strbuf_u32(&sb, MegaOhms, 1);
                        strbuf_char(&sb, '.');
                        strbuf_u32(&sb, kiloOhms, 3);
                        strbuf_str(&sb, " L");
                    }
                    if (kiloOhms) {
                        strbuf_u32(&sb, kiloOhms, 1);
                        strbuf_char(&sb, '.');
                        strbuf_u32(&sb, Ohms, 1);
                        strbuf_str(&sb, " j");
                    }
                    if (Ohms) {
                        strbuf_u32(&sb, Ohms, 1);
                        strbuf_char(&sb, ' ');
                    }
                    strbuf_str(&sb, "Nl");
                    screen_get_center_from_string(&cur_x, sb.len);
                    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
                    break;
                }
                    
                case Ommeter_Error:
                default:
                    strbuf_str(&sb, "Measurement error");
                    screen_get_center_from_string(&cur_x, sb.len);
                    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
                    break;
            }
            proccessor_raise_event(DataOutputed);
            break; 
        }
        default: break;
    }

    screen_get_center_from_string(&cur_x, strlen(SETTINGS_CYRILIC));
    fb_draw_string_xy(cur_x, options_strg[2].y, SETTINGS_CYRILIC);

    cursor_draw_highlight();
    return;
}

ScreenID screen_main_handle_event (InputEvent event) {
    switch (test_config.mode_id) {
        case CheckDevice: 
        case VoltageGenerator:
            switch (event) {
                case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenMain;
                case InputEvent_ButtonPressed_Cancel: return ScreenMain;
                case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenMain;
                case InputEvent_ButtonPressed_Enter: {
                    uint8_t cur_option = cursor_get_current_selected_index();
                    switch (cur_option) {
                        case 0: return ((test_config.mode_id == CheckDevice) ? 
                                        (ScreenTesting) : 
                                        (ScreenGenerator));
                        case 1: return ScreenSettings;
                        default: return ScreenMain;
                    }
                }
                default: return ScreenMain;
            }

        case Voltmeter: 
        case Ommeter: 
            switch (event) {
                case InputEvent_ButtonPressed_Enter: return ScreenSettings;
                case InputEvent_ButtonPressed_Cancel:
                default: 
                    return ScreenMain;
            }

        default: return ScreenMain;
    }
}

void screen_main_on_exit (void) {
    return;
}