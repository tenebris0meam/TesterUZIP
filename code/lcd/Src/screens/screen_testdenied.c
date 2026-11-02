#include "screens/screen_testdenied.h"
#include "delay.h"
#include "strings_def.h"
#include "framebuffer.h"
#include "fonts.h"
#include "model.h"

static const uint32_t delay_testdenied_ms = 1000;
static uint8_t delay_task;

void screen_testdenied_on_enter (void) {
    (void)delay_register_task(&delay_task, delay_testdenied_ms, Millis);
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
    if (test_runtime.current_device->connector_id == RJ45) {
        fb_set_font(cgxyz.font_lat, 
                    cgxyz.width_px, 
                    cgxyz.height_px, 
                    FONT_FIRST_CHAR, 
                    FONT_LAST_CHAR);
    }
    strbuf_str(&sb, screen_interface_name(test_runtime.current_device->connector_id));
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    if (test_runtime.current_device->connector_id == RJ45) {
        fb_set_font(cgxyz.font_cyr, 
                    cgxyz.width_px, 
                    cgxyz.height_px, 
                    FONT_FIRST_CHAR, 
                    FONT_LAST_CHAR);
    }
    cur_y += 2 * cgxyz.height_px;
    // Пропускаем 2 строки для красоты

    strbuf_clear(&sb);
    strbuf_str(&sb, DENIED_BY_USER_CYRILIC);
    screen_get_center_from_string(&cur_x, sb.len);
    fb_draw_string_xy(cur_x, cur_y, strbuf_cstr(&sb));
    return;
}

ScreenID screen_testdenied_handle_event (InputEvent event) {
    return ((delay_is_not_yet_expired(delay_task)) ? ScreenTestDenied : ScreenMain);
}