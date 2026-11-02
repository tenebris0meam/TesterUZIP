#include "framebuffer.h"
#include "lcd.h"

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Буфер одной полосы - единственная память кадра в ОЗУ.
 */
static uint8_t band[FB_WIDTH];

/** @brief Номер полосы, которая сейчас в буфере. */
static uint8_t band_bank = 0;

/** @brief Верхняя строка текущей полосы. */
static uint8_t band_top = 0;

/** @brief Текущий шрифт. */
static const uint8_t *font_table = 0;
static uint8_t font_width = 0;
static uint8_t font_height = 0;
static uint8_t font_first = 0;
static uint8_t font_last = 0;

/** @brief Байт на колонку глифа: высота шрифта, округлённая вверх до байта. */
static uint8_t font_bytes_high = 0;

/** @brief Байт на глиф: байт ширины плюс колонки. */
static uint8_t font_bytes_per_char = 0;



void fb_begin_band (uint8_t bank) {
    if (bank >= FB_BANKS) {
        bank = FB_BANKS - 1;
    }
    band_bank = bank;
    band_top = (uint8_t)(bank * FB_BAND_HEIGHT);
    for (uint8_t i = 0; i < FB_WIDTH; i++) {
        band[i] = 0;
    }
    return;
}

void fb_flush_band (void) {
    lcd_write_band(band_bank, band, FB_WIDTH);
    return;
}

void fb_clear_screen (void) {
    for (uint8_t bank = 0; bank < FB_BANKS; bank++) {
        fb_begin_band(bank);
        fb_flush_band();
    }
    return;
}

void fb_set_font (const uint8_t *table, 
                  uint8_t width, 
                  uint8_t height, 
                  uint8_t first, 
                  uint8_t last) {
    font_table = table;
    font_width = width;
    font_height = height;
    font_first = first;
    font_last = last;
    font_bytes_high = (uint8_t)((height + 7u) / 8u);
    font_bytes_per_char = (uint8_t)(width * font_bytes_high + 1u);
    return;
}

void fb_set_pixel (uint8_t x, uint8_t y, bool black) {
    if ((x >= FB_WIDTH) || (y >= FB_HEIGHT)) {
        return;
    }

    /* Пиксель не из текущей полосы рисовать некуда */
    if ((y >> 3) != band_bank) {
        return;
    }

    uint8_t mask = (uint8_t)(1u << (y & 7u));
    if (black) {
        band[x] |= mask;
    } else {
        band[x] &= (uint8_t)(~mask);
    }
    return;
}

void fb_invert_area (uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    if ((w == 0) || (h == 0) || (x >= FB_WIDTH) || (y >= FB_HEIGHT)) {
        return;
    }

    /* Считаем в 16 битах: x + w и y + h легко переполняют байт */
    uint16_t y_beg = y;
    uint16_t y_end = (uint16_t)y + h;
    uint16_t band_beg = band_top;
    uint16_t band_end = (uint16_t)band_top + FB_BAND_HEIGHT;

    if (y_end > FB_HEIGHT) {
        y_end = FB_HEIGHT;
    }
    if ((y_beg >= band_end) || (y_end <= band_beg)) {
        return;
    }

    if (y_beg < band_beg) {
        y_beg = band_beg;
    }
    if (y_end > band_end) {
        y_end = band_end;
    }

    /* Одной маской на всю полосу вместо прохода по каждому пикселю */
    uint8_t mask = 0;
    for (uint16_t row = y_beg; row < y_end; row++) {
        mask |= (uint8_t)(1u << (row & 7u));
    }

    uint16_t x_end = (uint16_t)x + w;
    if (x_end > FB_WIDTH) {
        x_end = FB_WIDTH;
    }
    for (uint16_t col = x; col < x_end; col++) {
        band[col] ^= mask;
    }
    return;
}

uint8_t fb_draw_char_xy (uint8_t x, uint8_t y, char c) {
    if (font_table == 0) {
        return 0;
    }

    uint8_t code = (uint8_t)c;
    if ((code < font_first) || (code > font_last)) {
        code = (uint8_t)'.';
    }

    const uint8_t *glyph = font_table + 
                           (uint32_t)(code - font_first) * font_bytes_per_char;

    /* Первый байт глифа - его фактическая ширина */
    uint8_t var_width = glyph[0];
    if (var_width > font_width) {
        var_width = font_width;
    }
    glyph++;

    /* Символ, не помещающийся на экран, не рисуем вовсе */
    if (((uint16_t)x + var_width > FB_WIDTH) || 
        ((uint16_t)y + font_height > FB_HEIGHT)) {
        return 0;
    }

    /* Символ целиком вне полосы: рисовать нечего, но ширину вернуть надо */
    if (((uint16_t)y >= (uint16_t)band_top + FB_BAND_HEIGHT) || 
        ((uint16_t)y + font_height <= (uint16_t)band_top)) {
        return var_width;
    }

    for (uint8_t col = 0; col < var_width; col++) {
        for (uint8_t part = 0; part < font_bytes_high; part++) {
            uint8_t data = glyph[col * font_bytes_high + part];
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t row = (uint8_t)(part * 8u + bit);
                if (row >= font_height) {
                    break;
                }
                fb_set_pixel((uint8_t)(x + col), 
                             (uint8_t)(y + row), 
                             (bool)((data >> bit) & 1u));
            }
        }
    }
    return var_width;
}

void fb_draw_string_xy (uint8_t x, uint8_t y, const char *str) {
    if ((str == 0) || (font_table == 0)) {
        return;
    }

    uint8_t start_x = x;
    while (*str) {
        if (*str == '\n') {
            y += font_height + 2;
            x = start_x;
            str++;
            continue;
        }

        if ((uint16_t)y + font_height > FB_HEIGHT) {
            /* Строка не помещается по высоте */
            return;
        }

        uint8_t width = fb_draw_char_xy(x, y, *str);
        if (width == 0) {
            /*
             * Символ не поместился по ширине - обрываем строку целиком.
             * Продолжать нельзя: следующий символ может оказаться уже, чем
             * пропущенный, поместиться и встать не на своё место.
             */
            return;
        }
        uint16_t next = (uint16_t)((uint16_t)x + width + 1u);
        if (next >= FB_WIDTH) {
            /* Дальше по строке всё равно ничего не поместится */
            return;
        }
        x = (uint8_t)next;
        str++;
    }
    return;
}

void fb_draw_string_centered_xy (uint8_t x, uint8_t max_w, uint8_t y, const char *str) {
    if ((str == 0) || (font_table == 0)) {
        return;
    }

    const char *line_start = str;
    while (*line_start) {
        // Ищем конец текущей строки
        const char *line_end = line_start;
        while (*line_end && (*line_end != '\n')) {
            line_end++;
        }

        // Считаем ширину текущей строки в пикселях
        uint16_t line_w = 0;
        for (const char *p = line_start; p < line_end; p++) {
            uint8_t code = (uint8_t)*p;
            if ((code < font_first) || (code > font_last)) {
                code = (uint8_t)'.';
            }
            const uint8_t *glyph = font_table + (uint32_t)(code - font_first) * font_bytes_per_char;
            uint8_t var_width = glyph[0];
            if (var_width > font_width) {
                var_width = font_width;
            }
            line_w += var_width + 1;
        }
        // последний пиксель между глифами не считаем
        if (line_w) {
            line_w--;
        }

        // вычисляем координату х левого верхнего пикселя
        // отцентрованной строки длины line_w
        uint8_t start_x = x;
        if (line_w < max_w) {
            start_x = x + (max_w - line_w) / 2;
        }

        for (const char *p = line_start; p < line_end; p++) {
            uint8_t w = fb_draw_char_xy(start_x, y, *p);
            if (!w) {
                // если не влезло, не рисуем строку
                break;
            }
            start_x += w + 1;
        }

        // переходим на следующую строку
        if (*line_end == '\n') {
            y += font_height;
            line_start = line_end + 1;
        } else {
            break;
        }
    }
    return;
}

void fb_draw_bitmap (uint8_t x, 
                     uint8_t y, 
                     uint8_t w, 
                     uint8_t h, 
                     const uint8_t *bitmap) {
    if ((bitmap == 0) || (w == 0) || (h == 0) || (x >= FB_WIDTH)) {
        return;
    }

    uint8_t src_banks = (uint8_t)((h + 7u) / 8u);
    int16_t dst_top = (int16_t)band_top;

    for (uint8_t src_bank = 0; src_bank < src_banks; src_bank++) {
        int16_t src_top = (int16_t)((int16_t)y + (int16_t)(src_bank * 8u));
        int16_t shift = src_top - dst_top;

        /* Байт источника не пересекается с полосой */
        if ((shift <= -FB_BAND_HEIGHT) || (shift >= FB_BAND_HEIGHT)) {
            continue;
        }

        /* В последнем байте источника значимы не все биты */
        uint8_t valid = 0xFFu;
        uint16_t rows_before = (uint16_t)src_bank * 8u;
        if (rows_before + 8u > h) {
            valid = (uint8_t)((1u << (h - rows_before)) - 1u);
        }

        for (uint8_t col = 0; col < w; col++) {
            uint16_t dst_col = (uint16_t)x + col;
            if (dst_col >= FB_WIDTH) {
                break;
            }
            uint8_t data = (uint8_t)(bitmap[(uint16_t)src_bank * w + col] & valid);
            band[dst_col] |= (shift >= 0) ? 
                             (uint8_t)(data << shift) : 
                             (uint8_t)(data >> (-shift));
        }
    }
    return;
}
