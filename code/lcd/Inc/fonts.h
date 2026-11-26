#ifndef FONTS_H_
#define FONTS_H_

#include <stdint.h>

#define FONT_FIRST_CHAR (0x20)
#define FONT_LAST_CHAR (0x7F)
#define FONT_CHAR_COUNT (FONT_LAST_CHAR + 1 - FONT_FIRST_CHAR)
#define FONT_BYTES_PER_CHAR(width) (1 + 2*width)

/** @brief Шрифт: размер знакоместа и таблицы глифов. */
typedef struct {
    uint8_t width_px;  /**< Ширина знакоместа, пикселей */
    uint8_t height_px;  /**< Высота знакоместа, пикселей */
    const uint8_t *font_cyr;  /**< Таблица с кириллицей в ASCII-кодах (docs/markdown/character_table.md) */
    const uint8_t *font_lat;  /**< Таблица с латиницей */
} Font_TypeDef;

extern const Font_TypeDef cgxyz;
extern const Font_TypeDef jetbrains;

#endif /* FONTS_H_ */