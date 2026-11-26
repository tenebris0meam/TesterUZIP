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
static ScreenID previous_screen = ScreenHello;
static DisplayUpdatePolicy update_policy = DisplayUpdateEnable;
static bool is_screen_dirty;

static inline void display_draw_buffer (void) {
    for (uint8_t bank_id = 0; bank_id < FB_BANKS; bank_id++) {
        fb_begin_band(bank_id);
        screens[current_screen].render();
        fb_flush_band();
    }
    return;
}

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
    if (!linebuffer) {
        return false;
    }
    if (!options_init(workspace_start_x, 
                      workspace_start_y, 
                      FB_WIDTH, 
                      workspace_height_in_px, 
                      workspace_height_in_chars)) {
        return false;
    }
    inputevent_enable_init();
    return true;
}

static void display_proccess_input (void) {
    inputevent_routine();

    InputEvent cur_event = inputevent_get_state();
    if (cur_event != InputEvent_Count) {
        ScreenID next_screen = screens[current_screen].handle_event(cur_event);
        if (next_screen != current_screen) {
            previous_screen = current_screen;
            current_screen = next_screen;
            is_screen_dirty = true;
        }
    }
    return;
}

static void display_refresh (void) {
    if (screens[previous_screen].on_exit) {
        screens[previous_screen].on_exit();
    }
    screens[current_screen].on_enter();
    return;
}

void display_routine (void) {
    display_proccess_input();

    if (update_policy == DisplayUpdateSuspended) {
        return;
    }
    if (is_screen_dirty) {
        display_refresh();
        is_screen_dirty = false;
    }

    display_draw_buffer();
    return;
}

ScreenID display_get_current_screen (void) {
    return current_screen;
}

void display_set_updatepolicy (const DisplayUpdatePolicy new_policy) {
    update_policy = new_policy;
    return;
}