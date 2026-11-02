#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @file framebuffer.h
 * @brief Построчный (полосный) вывод на монохромный экран 128x64.
 *
 * @details Полный кадр в ОЗУ не хранится. Экран делится на FB_BANKS полос по
 *          8 пикселей высотой; в ОЗУ живёт ровно одна полоса - FB_WIDTH байт.
 *          Кадр рисуется так:
 *
 *          @code
 *          for (uint8_t bank = 0; bank < FB_BANKS; bank++) {
 *              fb_begin_band(bank);
 *              render();          // те же самые вызовы отрисовки
 *              fb_flush_band();
 *          }
 *          @endcode
 *
 *          Функции рисования сами отбрасывают всё, что не попадает в текущую
 *          полосу, поэтому код экранов писать по-особому не нужно.
 */

/** @brief Ширина экрана, пикселей. */
#define FB_WIDTH (128)

/** @brief Высота экрана, пикселей. */
#define FB_HEIGHT (64)

/** @brief Высота одной полосы, пикселей. Задана контроллером. */
#define FB_BAND_HEIGHT (8)

/** @brief Количество полос в кадре. */
#define FB_BANKS (FB_HEIGHT / FB_BAND_HEIGHT)

/** @brief Цвет пикселя: включённый (тёмный). */
#define FB_BLACK (true)

/** @brief Цвет пикселя: выключенный (светлый). */
#define FB_WHITE (false)



/**
 * @brief Начать полосу: очистить буфер и запомнить её номер.
 * @param bank номер полосы, 0..FB_BANKS-1
 */
void fb_begin_band (uint8_t bank);

/** @brief Отправить текущую полосу в контроллер. */
void fb_flush_band (void);

/** @brief Залить весь экран белым, полоса за полосой. */
void fb_clear_screen (void);

/**
 * @brief Выбрать шрифт.
 * @param table  таблица глифов в формате MikroElektronika
 * @param width  ширина знакоместа
 * @param height высота знакоместа
 * @param first  код первого символа таблицы
 * @param last   код последнего символа таблицы
 */
void fb_set_font (const uint8_t *table, 
                  uint8_t width, 
                  uint8_t height, 
                  uint8_t first, 
                  uint8_t last);

/**
 * @brief Поставить пиксель.
 * @details Пиксели вне экрана и вне текущей полосы молча отбрасываются.
 */
void fb_set_pixel (uint8_t x, uint8_t y, bool black);

/** @brief Инвертировать прямоугольник (используется для выделения курсором). */
void fb_invert_area (uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Нарисовать символ.
 * @return фактическую ширину глифа или 0, если символ не помещается на экран.
 */
uint8_t fb_draw_char_xy (uint8_t x, uint8_t y, char c);

/** @brief Нарисовать строку, завершённую нулём. */
void fb_draw_string_xy (uint8_t x, uint8_t y, const char *str);

/**
 * @brief Нарисовать многострочную строку с выравниванием по центру.
 * @param x      левый край области (обычно workspace_start_x)
 * @param max_w  ширина области для центрирования (обычно workspace_width_in_px)
 * @param y      верхний край
 * @param str    строка (может содержать '\n')
 */
void fb_draw_string_centered_xy (uint8_t x, uint8_t max_w, uint8_t y, const char *str);

/**
 * @brief Нарисовать монохромное изображение.
 * @param x      левый край
 * @param y      верхний край
 * @param w      ширина изображения, пикселей
 * @param h      высота изображения, пикселей
 * @param bitmap данные: байт [bank * w + x] - 8 пикселей по вертикали,
 *               бит b - строка bank * 8 + b
 */
void fb_draw_bitmap (uint8_t x, 
                     uint8_t y, 
                     uint8_t w, 
                     uint8_t h, 
                     const uint8_t *bitmap);

#endif /* FRAMEBUFFER_H_ */
