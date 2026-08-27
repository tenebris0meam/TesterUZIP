#ifndef LCDFONT7X14_H_
#define LCDFONT7X14_H_

#include <stdint.h>

/**
 * @file LCDFont7x14.h
 * @brief Шрифт JetBrains Mono 7x13 в формате MikroElektronika.
 */

/** @brief Ширина знакоместа, пикселей. */
#define LCDFONT_WIDTH (7)

/** @brief Высота знакоместа, пикселей. */
#define LCDFONT_HEIGHT (13)

/** @brief Первый символ таблицы. */
#define LCDFONT_FIRST_CHAR (0x20)

/** @brief Последний символ таблицы. */
#define LCDFONT_LAST_CHAR (0x7F)

/** @brief Число символов в таблице. */
#define LCDFONT_CHAR_COUNT (LCDFONT_LAST_CHAR - LCDFONT_FIRST_CHAR + 1)

/** @brief Байт на символ: байт ширины плюс по 2 байта на колонку. */
#define LCDFONT_BYTES_PER_CHAR (1 + LCDFONT_WIDTH * 2)

/**
 * @brief Кириллическая раскладка: коды 0x40..0x7F - буквы кириллицы.
 * @details Соответствие ASCII-кодов буквам - docs/markdown/character_table.md.
 */
extern const uint8_t JetBrains_Mono7x13_Cyrilic[LCDFONT_BYTES_PER_CHAR * LCDFONT_CHAR_COUNT];

/** @brief Латинская раскладка: обычный ASCII. */
extern const uint8_t JetBrains_Mono7x13_Latin[LCDFONT_BYTES_PER_CHAR * LCDFONT_CHAR_COUNT];

#endif /* LCDFONT7X14_H_ */
