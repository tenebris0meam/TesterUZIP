#ifdef TESTER_MODES
#include "screens/screen_generator_generation.h"
#include "calc.h"
#include "framebuffer.h"
#include "fonts.h"
#include "hv_feedback.h"
#include "proccessor.h"
#include "states/generation.h"
#include "strbuf.h"
#include "strings_def.h"
#include <string.h>

static void draw_centered_generator(const char *text, uint8_t y) {
    uint8_t x;
    screen_get_center_from_string(&x, (uint8_t)strlen(text));
    fb_draw_string_xy(x, y, text);
    return;
}

void screen_generator_generation_on_enter(void) {
    return;
}

void screen_generator_generation_on_exit(void) {
    return;
}

void screen_generator_generation_render(void) {
    StrBuf_TypeDef sb;
    uint8_t x;
    int32_t voltage_uV = 0, current_nA = 0;
    calc_get_test_data(&voltage_uV, &current_nA);
    draw_centered_generator(GENERATOR_CYRILIC, workspace_start_y);

    strbuf_init(&sb, linebuffer, linebuffer_size);
    strbuf_str(&sb, "Sqr`bj`: ");
    strbuf_u32(&sb, proccessor_get_manual_target_voltage(), 1U);
    strbuf_str(&sb, " B");
    screen_get_center_from_string(&x, (uint8_t)sb.len);
    fb_draw_string_xy(x, workspace_start_y + cgxyz.height_px * 2U, strbuf_cstr(&sb));

    int32_t voltage_before_div_uV = voltage_uV * HV_FEEDBACK_DIVIDER;
    int32_t voltage_V = voltage_before_div_uV / 1000000L;
    int32_t voltage_mV = (voltage_before_div_uV % 1000000L) / 1000L;
    strbuf_clear(&sb);
    strbuf_u32(&sb, voltage_V, 1U);
    strbuf_char(&sb, '.');
    strbuf_u32(&sb, voltage_mV, 3);
    strbuf_str(&sb, " B");
    screen_get_center_from_string(&x, (uint8_t)sb.len);
    fb_draw_string_xy(x, workspace_start_y + cgxyz.height_px * 4U, strbuf_cstr(&sb));

    int32_t current_mA = current_nA / 1000000L;
    int32_t current_uA = (current_nA % 1000000L) / 1000L;
    strbuf_clear(&sb);
    if (current_nA < 0L) {
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
    screen_get_center_from_string(&x, (uint8_t)sb.len);
    fb_draw_string_xy(x, workspace_start_y + cgxyz.height_px * 5U, strbuf_cstr(&sb));
    draw_centered_generator("M`g`d = b{jk~whr| cemep`rnp", workspace_start_y + cgxyz.height_px * 6U);
    return;
}

ScreenID screen_generator_generation_handle_event(InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Cancel:
            proccessor_request_mode_exit();
            return ScreenMain;
        default: return ScreenGeneratorGeneration;
    }
}
#endif