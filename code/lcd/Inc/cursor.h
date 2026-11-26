#ifndef CURSOR_H_
#define CURSOR_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Прямоугольник одного пункта списка на экране. */
typedef struct {
    uint8_t x;  /**< Левый край, пикселей */
    uint8_t y;  /**< Верхний край, пикселей */
    uint8_t w;  /**< Ширина, пикселей */
    uint8_t h;  /**< Высота, пикселей */
} OptionRect_TypeDef;

/** @brief Состояние курсора в списке. */
typedef struct {
    const OptionRect_TypeDef *options;  /**< Прямоугольники пунктов списка */
    uint8_t count;  /**< Число пунктов */
    uint8_t selected_index;  /**< Индекс выбранного пункта */
    uint8_t window_first;  /**< Индекс первого видимого пункта (прокрутка) */
} Cursor_TypeDef;

void cursor_init (const OptionRect_TypeDef *options, 
                  uint8_t amount_options, 
                  uint8_t initial_index, 
                  uint8_t first_item);
bool options_init (uint8_t start_x_px, 
                   uint8_t start_y_px, 
                   uint8_t workspace_width_px, 
                   uint8_t workspace_height_px,
                   uint8_t workspace_height_chars);
bool cursor_move (int8_t direction);
const OptionRect_TypeDef *cursor_get_current_bounding_box (void);
uint8_t cursor_get_current_selected_index (void);

/**
 * @brief Подсветить выбранный пункт короткого меню.
 * @details Годится там, где пунктов не больше CURSOR_VISIBLE_ROWS и каждый
 *          пункт стоит на своей строке. Для длинных списков с прокруткой
 *          использовать cursor_draw_highlight_row().
 */
void cursor_draw_highlight (void);

/**
 * @brief Подсветить строку экрана по её номеру, 0..CURSOR_VISIBLE_ROWS-1.
 * @details Нужна для списков длиннее трёх пунктов: там номер выбранного
 *          элемента и номер строки на экране - разные вещи.
 */
void cursor_draw_highlight_row (uint8_t row);

/**
 * @brief Номер первого видимого элемента списка при прокрутке.
 * @details Окно из CURSOR_VISIBLE_ROWS строк сдвигается так, чтобы выбранный
 *          элемент всегда оставался виден.
 */
uint8_t cursor_get_window_first (void);

void cursor_set_options_amount (uint8_t options_amount_new);

extern Cursor_TypeDef cursor;
extern OptionRect_TypeDef *options_strg;
extern uint8_t options_amount;

#endif /* CURSOR_H_ */
