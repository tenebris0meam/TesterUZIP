#include "screens/screen_testdenied.h"
#include "input_events.h"
#include "screens.h"
#include "strings_def.h"
#include "framebuffer.h"
#include "fonts.h"

void screen_testdenied_on_enter (void) {
    inputevent_enable_testdenied();
    return;
}

void screen_testdenied_on_exit (void) {
    return;
}

void screen_testdenied_render (void) {
    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    uint8_t cur_x;
    uint8_t cur_y = workspace_start_y;

    strbuf_str(&sb, TO_TEST_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
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
    strbuf_str(&sb, DENIED_BY_USER_CYRILIC);
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
    return;
}

ScreenID screen_testdenied_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_TestDeniedScreenTimeout: return ScreenMain;
        default: return ScreenTestDenied;
    }
}