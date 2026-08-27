#ifndef LCD_H_
#define LCD_H_

#include "stm32f103xb.h"

#include <stdint.h>

/**
 * @file lcd.h
 * @brief Драйвер контроллера NT75451 (монохромный экран 128x64) по SPI2.
 *
 * @details Модуль знает про контроллер и шину; про содержимое кадра он не знает
 *          ничего - рисованием занимается framebuffer.c, который отдаёт сюда
 *          готовые полосы по 128 байт.
 */

/** @brief Шина, на которой висит экран. Делится с INA239 и цифровым потенциометром. */
#define LCD_SPI SPI2

/** @brief Длительность импульса сброса контроллера, мс. */
#define LCD_RESET_TIME_MS (9)

/** @brief Длительность восстановления сброса дисплея */
#define LCD_RESTORE_RESET_TIME_MS (40)

/** @brief Ширина экрана в столбцах (равна FB_WIDTH). */
#define LCD_COLUMNS (128)

/** @brief Число полос по 8 пикселей (равно FB_BANKS). */
#define LCD_PAGES (8)

/** @brief Аппаратный сброс и инициализация контроллера. */
void lcd_init (void);

void lcd_reset (void);

/** @brief Послать команду контроллеру (A0 = 0). */
void lcd_command (uint8_t cmd);

/**
 * @brief Выгрузить одну полосу на экран (A0 = 1).
 * @param bank номер полосы 0..LCD_PAGES-1
 * @param data данные полосы
 * @param len  сколько столбцов выгружать, не больше LCD_COLUMNS
 */
void lcd_write_band (uint8_t bank, const uint8_t *data, uint8_t len);

#endif /* LCD_H_ */
