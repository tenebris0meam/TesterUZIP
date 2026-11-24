#include "screens/screen_multipole_wait.h"
#include "framebuffer.h"
#include "fonts.h"
#include "model.h"
#include "strbuf.h"

static void screen_multipole_wait_draw_line(uint8_t y, const char *text)
{
    fb_draw_string_centered_xy(workspace_start_x,
                                workspace_width_in_px,
                                y,
                                text);
}

void screen_multipole_wait_on_enter(void) {}
void screen_multipole_wait_on_exit(void) {}

void screen_multipole_wait_render(void)
{
    StrBuf_TypeDef sb;
    uint8_t y = workspace_start_y;
    strbuf_init(&sb, linebuffer, linebuffer_size);

    screen_multipole_wait_draw_line(y, "Ondjk~whre yso{");
    y += cgxyz.height_px;

    if (test_runtime.current_sequence != 0) {
        strbuf_str(&sb, "Y1 j ");
        strbuf_u32(&sb, (uint32_t)test_runtime.current_sequence->input_pin + 1U, 0);
        strbuf_str(&sb, ", Y2 j ");
        strbuf_u32(&sb, (uint32_t)test_runtime.current_sequence->output_pin + 1U, 0);
    } else {
        strbuf_str(&sb, "Y1/Y2: --");
    }
    screen_multipole_wait_draw_line(y, strbuf_cstr(&sb));
    y += cgxyz.height_px;

    screen_multipole_wait_draw_line(y, "Jp`rjn g`ljmhre");
    y += cgxyz.height_px;
    screen_multipole_wait_draw_line(y, "yso{ dk qr`pr`");
}

ScreenID screen_multipole_wait_handle_event(InputEvent event)
{
    (void)event;
    return ScreenMultiPoleWait;
}
