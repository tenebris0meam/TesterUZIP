#include "screens/screen_result.h"
#include "strings_def.h"
#include "fonts.h"
#include "framebuffer.h"
#include "model.h"

void screen_result_on_enter (void) {
    return;
}

void screen_result_on_exit (void) {
    return;
}

void screen_result_render (void) {
    uint8_t cur_x = workspace_start_x;
    uint8_t cur_y = workspace_start_y;

    // Пропускаем первую линию
    cur_y += cgxyz.height_px;

    StrBuf_TypeDef sb;
    strbuf_init(&sb, linebuffer, linebuffer_size);
    strbuf_str(&sb, test_results.fail_was ? FAILED_CYRILIC : SUCCESS_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));

    // Пропускаем третью линию
    cur_y += cgxyz.height_px;

    strbuf_clear(&sb);
    strbuf_u32(&sb, test_results.passed_cnt, 3);
    strbuf_char(&sb, '/');
    strbuf_u32(&sb, model_get_total_tests(), 3);
    strbuf_char(&sb, ' ');
    strbuf_str(&sb, PASSED_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    return;
}

ScreenID screen_result_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Enter: 
            if (test_results.fail_was) {
                return ScreenCheckFailed;
            } else {
                return ScreenMain;
            }
        default: return ScreenResult;
    }
}