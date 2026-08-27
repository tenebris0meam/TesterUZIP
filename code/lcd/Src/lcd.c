#include "spi.h"
#include "lcd.h"
#include "delay.h"
#include "gpio.h"
#include <stddef.h>

/** @brief Прижать CS экрана. */
static inline void lcd_select (void) {
    Pin_Set_State(&nCS_LCD, false);
    return;
}

/** @brief Отпустить CS экрана. */
static inline void lcd_deselect (void) {
    Pin_Set_State(&nCS_LCD, true);
    return;
}

/** @brief A0 = 0: дальше идёт команда. */
static inline void lcd_mode_command (void) {
    Pin_Set_State(&LCD_ADDR0, false);
    return;
}

/** @brief A0 = 1: дальше идут данные. */
static inline void lcd_mode_data (void) {
    Pin_Set_State(&LCD_ADDR0, true);
    return;
}

/** @brief Выбрать страницу (полосу) 0..7. */
static void lcd_set_page (uint8_t page) {
    if (page >= LCD_PAGES) {
        page = LCD_PAGES - 1;
    }
    lcd_command((uint8_t)(0xB0u | page));
    return;
}

/** @brief Выбрать столбец: сначала младший полубайт, затем старший. */
static void lcd_set_column (uint8_t column) {
    lcd_command((uint8_t)(column & 0x0Fu));
    lcd_command((uint8_t)(((column >> 4) & 0x0Fu) | 0x10u));
    return;
}

void lcd_init (void) {
    lcd_reset();

    lcd_command(0x2F);
    lcd_command(0x27);
    lcd_command(0x89);
    lcd_command(0x00);
    lcd_command(0x81);
    lcd_command(0x30);
    lcd_command(0x40);
    lcd_command(0xA6);
    lcd_command(0xC8);
    lcd_command(0xA0);
    lcd_command(0xAF);
    return;
}

/** @brief Аппаратный сброс контроллера. */
void lcd_reset (void) {
    lcd_select();
    Pin_Set_State(&nRESET_LCD, false);
    delay_ms(LCD_RESET_TIME_MS);
    Pin_Set_State(&nRESET_LCD, true);
    delay_ms(LCD_RESTORE_RESET_TIME_MS);
    lcd_deselect();
    return;
}

/** @brief Сколько ждём шину и завершения обмена, мс. */
#define LCD_BUS_TIMEOUT_MS (20)

/**
 * @brief Взять шину под экран.
 * @details Ждём с ограничением по времени. Это главный цикл, ждать здесь
 *          можно: освобождают шину обработчики прерываний, они нас вытеснят.
 */
static bool lcd_take_bus (void) {
    uint8_t delay_id = 0;
    if (delay_register_task(&delay_id, LCD_BUS_TIMEOUT_MS, Millis)) {
        while (spi2ctrl.capture(LCD) != SPICtrlStatus_Allow) {
            if (!delay_is_not_yet_expired(delay_id)) {
                return false;
            }
        }
        delay_unregister_task(delay_id);
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Дождаться конца обмена и освобождения шины.
 */
static void lcd_finish (void) {
    (void)spi2ctrl.wait_done(LCD_BUS_TIMEOUT_MS);
    return;
}

void lcd_write_band (uint8_t bank, const uint8_t *data, uint8_t len) {
    if (data == 0) {
        return;
    }
    if (len > LCD_COLUMNS) {
        len = LCD_COLUMNS;
    }

    lcd_set_page(bank);
    lcd_set_column(0);

    if (!lcd_take_bus()) {
        return;
    }
    lcd_mode_data();
    spi2ctrl.operate(data, len, NULL, 0, LCD_BUS_TIMEOUT_MS);
    lcd_finish();
    return;
}

static volatile uint8_t cmd_storage = 0;

void lcd_command (uint8_t cmd) {
    cmd_storage = cmd;

    if (!lcd_take_bus()) {
        return;
    }
    lcd_mode_command();
    spi2ctrl.operate((const uint8_t *)&cmd_storage, 1, NULL, 0, LCD_BUS_TIMEOUT_MS);
    lcd_finish();
    return;
}
