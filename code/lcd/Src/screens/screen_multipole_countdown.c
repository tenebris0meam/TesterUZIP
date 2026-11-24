#include "screens/screen_multipole_countdown.h"
#include "framebuffer.h"
#include "fonts.h"
#include "model.h"
#include "strbuf.h"
#include "multi_pole_test.h"

static void screen_multipole_countdown_draw_line(uint8_t y, const char *text)
{
    fb_draw_string_centered_xy(workspace_start_x,
                                workspace_width_in_px,
                                y,
                                text);
}

void screen_multipole_countdown_on_enter(void) {}
void screen_multipole_countdown_on_exit(void) {}

void screen_multipole_countdown_render(void)
{
    StrBuf_TypeDef sb;
    uint8_t y = workspace_start_y;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    screen_multipole_countdown_draw_line(y, "P`gnljmhre yso{!");
    y += cgxyz.height_px;
    screen_multipole_countdown_draw_line(y, "Sqr`mnbhre Y1/Y2");
    y += cgxyz.height_px;

    if (test_runtime.current_sequence != 0) {
        strbuf_str(&sb, "M` khmhh ");
        strbuf_u32(&sb, (uint32_t)test_runtime.current_sequence->input_pin + 1U, 0);
        strbuf_char(&sb, '/');
        strbuf_u32(&sb, (uint32_t)test_runtime.current_sequence->output_pin + 1U, 0);
    } else {
        strbuf_str(&sb, "M` sj`g`mm{e khmhh");
    }
    screen_multipole_countdown_draw_line(y, strbuf_cstr(&sb));
    y += cgxyz.height_px;

    strbuf_clear(&sb);
    strbuf_str(&sb, "Qr`pr wepeg: ");
    strbuf_u32(&sb, multi_pole_test_countdown_remaining(), 0);
    screen_multipole_countdown_draw_line(y, strbuf_cstr(&sb));
}

ScreenID screen_multipole_countdown_handle_event(InputEvent event)
{
    (void)event;
    return ScreenMultiPoleCountdown;
}
