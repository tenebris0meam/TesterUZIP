#ifndef TACHYON_LOGO_H_
#define TACHYON_LOGO_H_

#include <stdint.h>

/**
 * @file tachyon_logo.h
 * @brief Логотип экрана приветствия.
 * @details Сам массив лежит в tachyon_logo.c. Раскладка совпадает с раскладкой
 *          кадрового буфера: байт [bank * LOGO_WIDTH + x] - 8 пикселей по
 *          вертикали, бит b - строка bank * 8 + b.
 */

/** @brief Ширина логотипа, пикселей. */
#define LOGO_WIDTH (64)

/** @brief Высота логотипа, пикселей. */
#define LOGO_HEIGHT (64)

extern const uint8_t logo[LOGO_WIDTH * LOGO_HEIGHT / 8];

#endif /* TACHYON_LOGO_H_ */
