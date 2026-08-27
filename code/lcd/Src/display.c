#include "display.h"
#include "framebuffer.h"
#include "lcd.h"
#include "LCDFont7x14.h"
#include "input_events.h"
#include "screens.h"

#include <stdint.h>

static ScreenID current_screen = ScreenHello;

bool display_init (void) {
    lcd_init();
    fb_set_font(JetBrains_Mono7x13_Cyrilic, 
                LCDFONT_WIDTH, 
                LCDFONT_HEIGHT, 
                LCDFONT_FIRST_CHAR, 
                LCDFONT_LAST_CHAR);
    fb_clear_screen();

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
    for (uint8_t bank = 0; bank < FB_BANKS; bank++) {
        fb_begin_band(bank);
        screens[current_screen].render();
        fb_flush_band();
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
