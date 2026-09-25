#include "cursor.h"
#include "framebuffer.h"
#include <stdlib.h>



Cursor_TypeDef cursor = { 0 };

// const OptionRect_TypeDef options_in_list[CURSOR_VISIBLE_ROWS] = {
//     { .x = 1, .y = 19, .w = 126, .h = 13 }, 
//     { .x = 1, .y = 32, .w = 126, .h = 13 }, 
//     { .x = 1, .y = 45, .w = 126, .h = 13 } 
// };


OptionRect_TypeDef *options_strg = 0;
uint32_t options_amount = 0;

void options_init (uint8_t start_x_px, 
                   uint8_t start_y_px, 
                   uint8_t workspace_width_px, 
                   uint8_t workspace_height_px,
                   uint8_t workspace_height_chars) {
    options_strg = (OptionRect_TypeDef *)malloc(sizeof(OptionRect_TypeDef) * workspace_height_chars);

    // Первая строка всегда отдаётся под заголовок, 
    // а вторая всегда пропускается, чтобы отделить заголовок от списка
    options_amount = workspace_height_chars - 2;
    uint8_t glyph_height_px = workspace_height_px / workspace_height_chars;
    for (uint8_t i = 0; i < options_amount; i++) {
        options_strg[i].x = start_x_px;
        options_strg[i].y = start_y_px + (i + 2) * glyph_height_px;
        options_strg[i].w = workspace_width_px;
        options_strg[i].h = glyph_height_px;
    }
    return;
}

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
    if (cursor.selected_index >= options_amount) {
        return 0;
    }
    return &cursor.options[cursor.selected_index];
}

uint8_t cursor_get_current_selected_index (void) {
    return cursor.selected_index;
}

uint8_t cursor_get_window_first (void) {
    if (cursor.selected_index >= options_amount) {
        return (uint8_t)(cursor.selected_index - options_amount + 1);
    }
    return 0;
}

void cursor_draw_highlight (void) {
    const OptionRect_TypeDef *cur_rect = cursor_get_current_bounding_box();
    if (cur_rect) {
        fb_invert_area(cur_rect->x - 1, cur_rect->y - 1, cur_rect->w, cur_rect->h);
    }
    return;
}

void cursor_draw_highlight_row (uint8_t row) {
    if (row >= options_amount) {
        return;
    }
    fb_invert_area(options_strg[row].x - 1, 
                   options_strg[row].y - 1, 
                   options_strg[row].w, 
                   options_strg[row].h);
    return;
}
