#include "screens/screen_runningtest.h"
#include "strings_def.h"
#include "model.h"
#include "framebuffer.h"
#include "fonts.h"

void screen_runningtest_on_enter (void) {
    /* Новый прогон по тому же прибору должен начинаться с чистых результатов */
    // model_reset_results();
    return;
}

void screen_runningtest_on_exit (void) {
    return;
}

void screen_runningtest_render (void) {
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
    screen_append_pins(&sb, test_results.current_sequence);
    strbuf_str(&sb, "      ");
    strbuf_u32(&sb, test_results.passed_cnt, 3);
    strbuf_char(&sb, '/');
    strbuf_u32(&sb, model_get_total_tests(), 3);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    return;
}

ScreenID screen_runningtest_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_TestSequenceComplete: return ScreenResult;
        default: return ScreenRunningTest;
    }
}