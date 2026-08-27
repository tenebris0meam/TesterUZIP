#ifndef CURSOR_H_
#define CURSOR_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Прямоугольник одного пункта списка на экране. */
typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
} OptionRect_TypeDef;

typedef struct {
    const OptionRect_TypeDef *options;
    uint8_t count;
    uint8_t selected_index;
} Cursor_TypeDef;

/** @brief Сколько строк списка помещается под заголовком экрана. */
#define CURSOR_VISIBLE_ROWS (3)

void cursor_init (const OptionRect_TypeDef *options, 
                  uint8_t amount_options, 
                  uint8_t initial_index);
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

extern Cursor_TypeDef cursor;
extern const OptionRect_TypeDef options_in_list[CURSOR_VISIBLE_ROWS];

#endif /* CURSOR_H_ */
