#include "cursor.h"
#include "framebuffer.h"



Cursor_TypeDef cursor = { 0 };

const OptionRect_TypeDef options_in_list[CURSOR_VISIBLE_ROWS] = {
    { .x = 1, .y = 19, .w = 126, .h = 13 }, 
    { .x = 1, .y = 32, .w = 126, .h = 13 }, 
    { .x = 1, .y = 45, .w = 126, .h = 13 } 
};



void cursor_init (const OptionRect_TypeDef *options, 
                  uint8_t amount_options, 
                  uint8_t initial_index) {
    cursor.options = options;
    cursor.count = amount_options;
    cursor.selected_index = (initial_index < amount_options) ? initial_index : 0;
    return;
}

bool cursor_move (int8_t direction) {
    if (!cursor.count) {
        return false;
    }

    if (direction < 0) {
        if (cursor.selected_index == 0) {
            return false;
        }
        cursor.selected_index--;
        return true;
    } else if (direction > 0) {
        if (cursor.selected_index == cursor.count - 1) {
            return false;
        }
        cursor.selected_index++;
        return true;
    } else {
        return true;
    }
}

const OptionRect_TypeDef *cursor_get_current_bounding_box (void) {
    /*
     * Индекс проверяем обязательно: в списках длиннее CURSOR_VISIBLE_ROWS
     * cursor.count больше размера options_in_list, и обращение по
     * selected_index читало бы за границей массива.
     */
    if ((cursor.count == 0) || (cursor.options == 0)) {
        return 0;
    }
    if (cursor.selected_index >= CURSOR_VISIBLE_ROWS) {
        return 0;
    }
    return &cursor.options[cursor.selected_index];
}

uint8_t cursor_get_current_selected_index (void) {
    return cursor.selected_index;
}

uint8_t cursor_get_window_first (void) {
    if (cursor.selected_index >= CURSOR_VISIBLE_ROWS) {
        return (uint8_t)(cursor.selected_index - CURSOR_VISIBLE_ROWS + 1);
    }
    return 0;
}

void cursor_draw_highlight (void) {
    const OptionRect_TypeDef *cur_rect = cursor_get_current_bounding_box();
    if (cur_rect) {
        fb_invert_area(cur_rect->x, cur_rect->y, cur_rect->w, cur_rect->h);
    }
    return;
}

void cursor_draw_highlight_row (uint8_t row) {
    if (row >= CURSOR_VISIBLE_ROWS) {
        return;
    }
    fb_invert_area(options_in_list[row].x, 
                   options_in_list[row].y, 
                   options_in_list[row].w, 
                   options_in_list[row].h);
    return;
}
