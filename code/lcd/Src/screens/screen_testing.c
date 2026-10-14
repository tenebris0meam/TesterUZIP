#include "screens/screen_testing.h"
#include "strings_def.h"
#include "fonts.h"
#include "framebuffer.h"
#include "proccessor.h"
#include "model.h"

void screen_testing_on_enter (void) {
    return;
}

void screen_testing_on_exit (void) {
    return;
}

void screen_testing_render (void) {
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
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    cur_y += 2 * cgxyz.height_px;
    // пропускаем 2 строки для красоты

    strbuf_clear(&sb);
    strbuf_str(&sb, INTERFACE_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    cur_y += cgxyz.height_px;

    strbuf_clear(&sb);
    strbuf_str(&sb, screen_interface_name(test_results.current_device->connector_id));
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    cur_y += 2 * cgxyz.height_px;
    // Пропускаем 2 строки для красоты

    strbuf_clear(&sb);
    strbuf_str(&sb, CONTINUE_CYRILIC);
    strbuf_char(&sb, '?');
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    return;
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