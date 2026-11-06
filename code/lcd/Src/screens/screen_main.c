#include "screens/screen_main.h"
#include "cursor.h"
#include "params.h"
#include "screens.h"
#include "strbuf.h"
#include "framebuffer.h"
#include "strings_def.h"
#include "fonts.h"
#include "model.h"

#include <string.h>

#ifdef TESTER_MODES
#include "generation.h"
#include "proccessor.h"
#include "resolver.h"
#include "calc.h"
#endif


void screen_main_on_enter (void) {
#ifndef TESTER_MODES
    if (model_is_valid()) {
        cursor_init(&options_strg[1], 2, 0, 0);
    } else {
        cursor_init(&options_strg[1], 1, 0, 0);
    }
#else
    switch (test_config.mode_id) {
        case CheckDevice: 
        case VoltageGenerator:
            cursor_init(&options_strg[1], 3, 0, 0);
            break;

        case Voltmeter: 
        case Ommeter: 
            cursor_init(&options_strg[3], 1, 0, 0);
            break;

        default: return;
    }
#endif
    return;
}

void screen_main_render (void) {
    uint8_t cur_x = 0;
    uint8_t cur_y = workspace_start_y;
    StrBuf_TypeDef sb;

    strbuf_init(&sb, linebuffer, linebuffer_size);
#ifdef TESTER_MODES
    switch (test_config.mode_id) {
        case CheckDevice: {
#endif
            if (model_is_valid()) {
                screen_append_device_name(&sb);
                fb_draw_string_centered_xy(workspace_start_x, 
                                            workspace_width_in_px, 
                                            cur_y, 
                                            strbuf_cstr(&sb));
                cur_y += cgxyz.height_px;
                strbuf_clear(&sb);

                const char *str_interface = screen_interface_name(test_runtime.current_device->connector_id);
                if (test_runtime.current_device->connector_id == RJ45) {
                    fb_set_font(cgxyz.font_lat, 
                                cgxyz.width_px, 
                                cgxyz.height_px, 
                                FONT_FIRST_CHAR, 
                                FONT_LAST_CHAR);
                }
                fb_draw_string_centered_xy(workspace_start_x, 
                                            workspace_width_in_px, 
                                            cur_y, 
                                            str_interface);
                if (test_runtime.current_device->connector_id == RJ45) {
                    fb_set_font(cgxyz.font_cyr, 
                                cgxyz.width_px, 
                                cgxyz.height_px, 
                                FONT_FIRST_CHAR, 
                                FONT_LAST_CHAR);
                }

                fb_draw_string_centered_xy(workspace_start_x, 
                                            workspace_width_in_px, 
                                            options_strg[1].y, 
                                            TO_TEST_CYRILIC);

                strbuf_str(&sb, CHOSING_CYRILIC);
                strbuf_char(&sb, ' ');
                strbuf_str(&sb, DEVICES_CYRILIC);
                fb_draw_string_centered_xy(workspace_start_x, 
                                            workspace_width_in_px, 
                                            options_strg[2].y, 
                                            strbuf_cstr(&sb));
                strbuf_clear(&sb);
            } else {
                strbuf_str(&sb, CHOSING_CYRILIC);
                strbuf_char(&sb, ' ');
                strbuf_str(&sb, DEVICES_CYRILIC);
                fb_draw_string_centered_xy(workspace_start_x, 
                                            workspace_width_in_px, 
                                            options_strg[1].y, 
                                            strbuf_cstr(&sb));
                strbuf_clear(&sb);
            }
#ifdef TESTER_MODES
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
            strbuf_clear(&sb);

            screen_get_center_from_string(&cur_x, strlen(GENERATION_CYRILIC));
            fb_draw_string_xy(cur_x, options_strg[1].y, GENERATION_CYRILIC);

            strbuf_str(&sb, SETTINGS_CYRILIC);
            strbuf_char(&sb, ' ');
            strbuf_str(&sb, GENERATORA_CYRILIC);
            screen_get_center_from_string(&cur_x, sb.len);
            fb_draw_string_xy(cur_x, options_strg[2].y, strbuf_cstr(&sb));
            strbuf_clear(&sb);

            strbuf_str(&sb, CHOSING_CYRILIC);
            strbuf_char(&sb, ' ');
            strbuf_str(&sb, MODEA_CYRILIC);
            screen_get_center_from_string(&cur_x, sb.len);
            fb_draw_string_xy(cur_x, options_strg[3].y, strbuf_cstr(&sb));
            strbuf_clear(&sb);
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
#endif
    cursor_draw_highlight();
    return;
}

ScreenID screen_main_handle_event (InputEvent event) {
#ifdef TESTER_MODES
    switch (test_config.mode_id) {
        case CheckDevice: 
        case VoltageGenerator:
#endif
            switch (event) {
                case InputEvent_ButtonPressed_Previous: cursor_move(-1); return ScreenMain;
                case InputEvent_ButtonPressed_Cancel: return ScreenMain;
                case InputEvent_ButtonPressed_Next: cursor_move(+1); return ScreenMain;
                case InputEvent_ButtonPressed_Enter: {
                    uint8_t cur_option = cursor_get_current_selected_index();
                    if (model_is_valid()) {
                        switch (cur_option) {
                            case 0: return ScreenTestConfirm;
                            case 1: return ScreenCheckdeviceSeries;
                            case 2: return ScreenModes;
                            default: return ScreenMain;
                        }
                    } else {
                        switch (cur_option) {
                            case 0: return ScreenCheckdeviceSeries;
                            case 1: return ScreenModes;
                            default: return ScreenMain;
                        }
                    }
                }
                default: return ScreenMain;
            }
#ifdef TESTER_MODES
        case Voltmeter: 
        case Ommeter: 
            switch (event) {
                case InputEvent_ButtonPressed_Enter: return ScreenModes;
                case InputEvent_ButtonPressed_Cancel:
                default: 
                    return ScreenMain;
            }

        default: return ScreenMain;
    }
#endif
}

void screen_main_on_exit (void) {
    return;
}