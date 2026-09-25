#include "display.h"
#include "cursor.h"
#include "framebuffer.h"
#include "lcd.h"
#include "input_events.h"
#include "screens.h"
#include "fonts.h"

#include <stdint.h>
#include <stdlib.h>

static ScreenID current_screen = ScreenHello;
static bool is_render_allowed = true;

bool display_init (void) {
    lcd_init();
    fb_set_font(cgxyz.font_cyr, 
                cgxyz.width_px, 
                cgxyz.height_px, 
                FONT_FIRST_CHAR, 
                FONT_LAST_CHAR);
    fb_clear_screen();

    workspace_start_x = 1;
    workspace_start_y = 1;
    workspace_width_in_px = FB_WIDTH - 2; // два крайних столбца - пустые
    workspace_height_in_px = FB_HEIGHT - 1; // первая строка - пустая
    workspace_width_in_chars = workspace_width_in_px / cgxyz.width_px;
    workspace_height_in_chars = workspace_height_in_px / cgxyz.height_px;

    linebuffer_size = workspace_width_in_px + 1; // дополнительный '\0' для си-строк
    linebuffer = (char *)malloc(linebuffer_size);

    options_init(workspace_start_x, 
                 workspace_start_y, 
                 FB_WIDTH, 
                 workspace_height_in_px, 
                 workspace_height_in_chars);

    inputevent_enable_init();

    current_screen = ScreenHello;
    screens[current_screen].on_enter();
    screens[current_screen].render();
    return true;
}

void display_routine (void) {
    inputevent_routine();

    InputEvent cur_state = inputevent_get_state();

    if (cur_state != InputEvent_Count) {
        ScreenID next_screen = screens[current_screen].handle_event(cur_state);
        if (next_screen != current_screen) {
            if (screens[current_screen].on_exit) {
                screens[current_screen].on_exit();
            }
            current_screen = next_screen;
            screens[current_screen].on_enter();
        }
    }

    /*
     * Кадр собирается полосами по 8 пикселей: в ОЗУ живёт только одна полоса,
     * поэтому render() вызывается FB_BANKS раз за кадр. Функции рисования сами
     * отбрасывают то, что не попало в текущую полосу, так что экранам об этом
     * знать не нужно. Очищать буфер отдельно не требуется - fb_begin_band()
     * делает это сам.
     */
    if (is_render_allowed) {
        for (uint8_t bank = 0; bank < FB_BANKS; bank++) {
            fb_begin_band(bank);
            screens[current_screen].render();
            fb_flush_band();
        }
    }
    return;
}

ScreenID display_get_current_screen (void) {
    return current_screen;
}

void display_set_screen (ScreenID screen) {
    if ((screen >= ScreenCount) || (screen == current_screen)) {
        return;
    }
    screens[current_screen].on_exit();
    current_screen = screen;
    screens[current_screen].on_enter();
    return;
}

void display_deny_render (void) {
    is_render_allowed = false;
    return;
}

void display_allow_render (void) {
    is_render_allowed = true;
    return;
}