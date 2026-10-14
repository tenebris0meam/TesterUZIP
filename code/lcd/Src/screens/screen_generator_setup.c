#include "screens/screen_generator_setup.h"
#include "framebuffer.h"
#include "fonts.h"
#include "hv_feedback.h"
#include "proccessor.h"
#include "screens.h"
#include "states/generation.h"
#include "strbuf.h"
#include <string.h>

static uint8_t selected_digit = 0U;
static uint16_t target_voltage_v = 1U;

static void draw_centered_setup(const char *text, uint8_t y) {
    uint8_t x;
    screen_get_center_from_string(&x, (uint8_t)strlen(text));
    fb_draw_string_xy(x, y, text);
}

void screen_generatorsetup_on_enter(void) {
    target_voltage_v = proccessor_get_manual_target_voltage();
    if (target_voltage_v < 1U) target_voltage_v = 1U;
    if (target_voltage_v > HV_FEEDBACK_MAX_TEST_V) target_voltage_v = HV_FEEDBACK_MAX_TEST_V;
    selected_digit = 0U;
}
void screen_generatorsetup_on_exit(void) { }

void screen_generatorsetup_render(void) {
    fb_set_font(cgxyz.font_lat, cgxyz.width_px, cgxyz.height_px,
                FONT_FIRST_CHAR, FONT_LAST_CHAR);
    char value[6] = { 'U', '=', '0', '0', '0', '\0' };
    uint8_t x;
    uint8_t y = workspace_start_y + cgxyz.height_px * 2U;
    value[2] = (char)('0' + (target_voltage_v / 100U) % 10U);
    value[3] = (char)('0' + (target_voltage_v / 10U) % 10U);
    value[4] = (char)('0' + target_voltage_v % 10U);

    fb_set_font(cgxyz.font_cyr, cgxyz.width_px, cgxyz.height_px,
                FONT_FIRST_CHAR, FONT_LAST_CHAR);
    draw_centered_setup("Sqr`bj` cemep`rnp`", workspace_start_y);
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);
    strbuf_str(&sb, value);
    strbuf_str(&sb, " B");
    screen_get_center_from_string(&x, sb.len);
    fb_draw_string_xy(x, y, strbuf_cstr(&sb));
    /* Highlight the selected numeric digit. */
    fb_invert_area((uint8_t)(x + (2U + selected_digit) * cgxyz.width_px),
                   (uint8_t)(y - 1U), cgxyz.width_px, cgxyz.height_px);
    draw_centered_setup("Bbepu/Bmhg: +/-", workspace_start_y + cgxyz.height_px * 4U);
    draw_centered_setup("Bbnd: qked./ophmr|", workspace_start_y + cgxyz.height_px * 5U);
    return;
}

ScreenID screen_generatorsetup_handle_event(InputEvent event) {
    uint16_t step = (selected_digit == 0U) ? 100U : ((selected_digit == 1U) ? 10U : 1U);
    switch (event) {
        case InputEvent_ButtonPressed_Previous:
            target_voltage_v = (target_voltage_v > step) ? (uint16_t)(target_voltage_v - step) : 1U;
            return ScreenGeneratorSetup;
        case InputEvent_ButtonPressed_Next:
            if (target_voltage_v < HV_FEEDBACK_MAX_TEST_V) {
                uint32_t candidate = (uint32_t)target_voltage_v + step;
                target_voltage_v = (uint16_t)((candidate > HV_FEEDBACK_MAX_TEST_V) ? HV_FEEDBACK_MAX_TEST_V : candidate);
            }
            return ScreenGeneratorSetup;
        case InputEvent_ButtonPressed_Enter:
            if (selected_digit < 2U) {
                selected_digit++;
                return ScreenGeneratorSetup;
            }
            proccessor_set_manual_target_voltage(target_voltage_v);
            proccessor_raise_event(EnterAccepted);
            return ScreenMain;
        case InputEvent_ButtonPressed_Cancel:
            return ScreenMain;
        default: return ScreenGeneratorSetup;
    }
}
