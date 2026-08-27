#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "screens.h"
#include <stdint.h>
#include <stdbool.h>

bool display_init (void);
ScreenID display_get_current_screen (void);

/**
 * @brief Принудительно перевести экран.
 * @details Нужно процессору: при аппаратной аварии экран должен смениться
 *          сам, без участия оператора. Отрабатывает on_exit прежнего экрана
 *          и on_enter нового, как при обычном переходе.
 */
void display_set_screen (ScreenID screen);
void display_routine (void);

#endif /* DISPLAY_H_ */