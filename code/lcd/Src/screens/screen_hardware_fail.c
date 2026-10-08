#include "screens/screen_hardware_fail.h"
#include "fonts.h"
#include "framebuffer.h"
#include "proccessor.h"

void screen_hardwarefail_on_enter (void) {
    return;
}

void screen_hardwarefail_on_exit (void) {
    return;
}

void screen_hardwarefail_render (void) {
    fb_set_font(cgxyz.font_lat, 
                cgxyz.width_px, 
                cgxyz.height_px, 
                FONT_FIRST_CHAR, 
                FONT_LAST_CHAR);

    uint8_t x = 1, y = 6;
    if (typefail & UnderCurrent) {
        fb_draw_string_xy(x, y, "Under Current");
        y += 13;
    }
    if (typefail & UnderVoltage) {
        fb_draw_string_xy(x, y, "Under Voltage");
        y += 13;
    }
    if (typefail & OverCurrent) {
        fb_draw_string_xy(x, y, "Over Current");
        y += 13;
    }
    if (typefail & OverVoltage) {
        fb_draw_string_xy(x, y, "Over Voltage");
        y += 13;
    }
    if (typefail & MathOverflow) {
        fb_draw_string_xy(x, y, "Math Overflow");
        y += 13;
    }
    if (typefail == NoFail) {
        fb_draw_string_xy(x, y, "No Fail");
        y += 13;
    }

    fb_set_font(cgxyz.font_cyr, 
                cgxyz.width_px, 
                cgxyz.height_px, 
                FONT_FIRST_CHAR, 
                FONT_LAST_CHAR);
    return;
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