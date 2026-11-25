#include "screens/screen_multipole_wait.h"
#include "framebuffer.h"
#include "fonts.h"
#include "input_events.h"
#include "model.h"
#include "screens.h"
#include "strbuf.h"
#include "proccessor.h"
#include "strings_def.h"

void screen_multipole_wait_on_enter(void) {
    return;
}

void screen_multipole_wait_on_exit(void) {
    return;
}

void screen_multipole_wait_render (void) {
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

    fb_draw_string_centered_xy(workspace_start_x,
                                workspace_width_in_px,
                                cur_y,
                                "Ondjk~whre yso{");
    cur_y += cgxyz.height_px;

    strbuf_clear(&sb);
    if (test_runtime.current_sequence != 0) {
        strbuf_str(&sb, PROBERED_CYRILIC);
        strbuf_str(&sb, " j ");
        strbuf_u32(&sb, (uint32_t)test_runtime.current_sequence->input_pin + 1U, 0);
        strbuf_str(&sb, ", ");
        strbuf_str(&sb, PROBEBLACK_CYRILIC);
        strbuf_str(&sb, " j ");
        strbuf_u32(&sb, (uint32_t)test_runtime.current_sequence->output_pin + 1U, 0);
    } else {
        strbuf_str(&sb, PROBES_CYRILIC);
        strbuf_str(&sb, ": --");
    }
    fb_draw_string_centered_xy(workspace_start_x,
                                workspace_width_in_px,
                                cur_y,
                                strbuf_cstr(&sb));
    cur_y += cgxyz.height_px;

    fb_draw_string_centered_xy(workspace_start_x,
                                workspace_width_in_px,
                                cur_y,
                                "Jp`rjn g`ljmhre");
    cur_y += cgxyz.height_px;
    fb_draw_string_centered_xy(workspace_start_x,
                                workspace_width_in_px,
                                cur_y,
                                "yso{ dk qr`pr`");
}

ScreenID screen_multipole_wait_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonTripplePressed_Cancel:
            proccessor_raise_event(TestDeniedByUser);
            return ScreenTestDenied;
        case InputEvent_SystemEvent: return ScreenMultiPoleCountdown;
        default: return ScreenMultiPoleWait;
    }
}
