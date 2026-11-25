#include "screens/screen_testrunning.h"
#include "input_events.h"
#include "proccessor.h"
#include "screens.h"
#include "strings_def.h"
#include "model.h"
#include "framebuffer.h"
#include "fonts.h"

void screen_testrunning_on_enter (void) {
    return;
}

void screen_testrunning_on_exit (void) {
    return;
}

void screen_testrunning_render (void) {
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    uint8_t cur_x;
    uint8_t cur_y = workspace_start_y;

    strbuf_str(&sb, TO_TEST_CYRILIC);
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
    cur_y += cgxyz.height_px;

    strbuf_clear(&sb);
    screen_append_device_name(&sb);
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
    cur_y += 2 * cgxyz.height_px;
    // пропускаем 2 строки для красоты

    strbuf_clear(&sb);
    screen_append_pins(&sb, test_runtime.current_sequence);
    strbuf_str(&sb, "      ");
    strbuf_u32(&sb, test_runtime.passed_cnt, 1);
    strbuf_char(&sb, '/');
    strbuf_u32(&sb, model_get_total_tests(), 1);
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
    return;
}

ScreenID screen_testrunning_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_HardwareFail: return ScreenHardwareFail;
        case InputEvent_SystemEvent: return ScreenMultiPoleWait;
        case InputEvent_TestSequenceComplete: return ScreenTestResult;
        case InputEvent_ButtonTripplePressed_Cancel:
            proccessor_raise_event(TestDeniedByUser);
            return ScreenTestDenied;
        default: return ScreenTestRunning;
    }
}