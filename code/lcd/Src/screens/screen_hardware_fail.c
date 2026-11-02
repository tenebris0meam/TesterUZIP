#include "screens/screen_hardware_fail.h"
#include "fonts.h"
#include "framebuffer.h"
#include "proccessor.h"
#include "screens.h"
#include "strbuf.h"
#include "strings_def.h"

static TypeFail typefail_screen = NoFail;

void screen_hardwarefail_on_enter (void) {
    typefail_screen = typefail;
    return;
}

void screen_hardwarefail_on_exit (void) {
    typefail_screen = NoFail;
    return;
}

void screen_hardwarefail_render (void) {
    uint8_t x = workspace_start_x, y = workspace_start_y;
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);
    strbuf_str(&sb, HARDWAREFAIL_CYRILIC);
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                y, 
                                strbuf_cstr(&sb));
    strbuf_clear(&sb);
    y += 2 * cgxyz.height_px;

    if (typefail_screen & OverCurrent) {
        strbuf_str(&sb, CURRENT_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, HIGHER_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, HIGHEGO_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, THAN_LIMIT_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, SENSORA_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;
    } else if (typefail_screen & OverVoltage) {
        strbuf_str(&sb, VOLTAGE_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, HIGHER_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, HIGHEGO_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, THAN_LIMIT_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, SENSORA_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;
    } else if (typefail_screen & UnderCurrent) {
        strbuf_str(&sb, CURRENT_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, LOWER_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, LOWEGO_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, THAN_LIMIT_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, SENSORA_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;
    } else if (typefail_screen & UnderVoltage) {
        strbuf_str(&sb, VOLTAGE_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, LOWER_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, LOWEGO_CYRILIC);
        strbuf_char(&sb, ' ');
        strbuf_str(&sb, THAN_LIMIT_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, SENSORA_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;
    } else if (typefail_screen & MathOverflow) {
        strbuf_str(&sb, OVERFLOW_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, REGISTERS_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, SENSORA_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;
    } else if (typefail_screen & MemoryError) {
        strbuf_str(&sb, MEMORY_ERROR_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;

        strbuf_str(&sb, SENSORA_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;
    } else if (typefail_screen == NoFail) {
        strbuf_str(&sb, "Meopedbhdemm` nxhaj`");
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    y, 
                                    strbuf_cstr(&sb));
        strbuf_clear(&sb);
        y += cgxyz.height_px;
    }

    return;
}

ScreenID screen_hardwarefail_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Enter: 
            if (typefail_screen == NoFail) {
                proccessor_raise_event(EnterAccepted);
                return ScreenMain;
            } else if (typefail_screen & OverCurrent) {
                typefail_screen &= (~OverCurrent);
            } else if (typefail_screen & OverVoltage) {
                typefail_screen &= (~OverVoltage);
            } else if (typefail_screen & UnderCurrent) {
                typefail_screen &= (~UnderCurrent);
            } else if (typefail_screen & UnderVoltage) {
                typefail_screen &= (~UnderVoltage);
            } else if (typefail_screen & MathOverflow) {
                typefail_screen &= (~MathOverflow);
            } else if (typefail_screen & MemoryError) {
                typefail_screen &= (~MemoryError);
            }
            return ScreenHardwareFail;
        default: return ScreenHardwareFail;
    }
}