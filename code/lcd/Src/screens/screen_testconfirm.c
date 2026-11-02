#include "screens/screen_testconfirm.h"
#include "screens.h"
#include "strings_def.h"
#include "fonts.h"
#include "framebuffer.h"
#include "proccessor.h"
#include "model.h"

void screen_testconfirm_on_enter (void) {
    return;
}

void screen_testconfirm_on_exit (void) {
    return;
}

void screen_testconfirm_render (void) {
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

    if (test_runtime.current_device->connector_id != (uint8_t)Probe) {
        strbuf_clear(&sb);
        strbuf_str(&sb, INTERFACE_CYRILIC);
        fb_draw_string_centered_xy(workspace_start_x, 
                                    workspace_width_in_px, 
                                    cur_y, 
                                    strbuf_cstr(&sb));
        cur_y += cgxyz.height_px;
    }

    strbuf_clear(&sb);
    if (test_runtime.current_device->connector_id == RJ45) {
        fb_set_font(cgxyz.font_lat, 
                    cgxyz.width_px, 
                    cgxyz.height_px, 
                    FONT_FIRST_CHAR, 
                    FONT_LAST_CHAR);
    }
    strbuf_str(&sb, screen_interface_name(test_runtime.current_device->connector_id));
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
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
    strbuf_str(&sb, CONTINUE_CYRILIC);
    strbuf_char(&sb, '?');
    fb_draw_string_centered_xy(workspace_start_x, 
                                workspace_width_in_px, 
                                cur_y, 
                                strbuf_cstr(&sb));
    return;
}

ScreenID screen_testconfirm_handle_event (InputEvent event) {
    switch (event) {
        case InputEvent_ButtonPressed_Cancel: return ScreenMain;
        case InputEvent_ButtonPressed_Enter: 
            proccessor_raise_event(EnterAccepted);
            return ScreenTestRunning;
        default: return ScreenTestConfirm;
    }
}