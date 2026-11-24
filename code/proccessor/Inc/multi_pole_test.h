#ifndef MULTI_POLE_TEST_H_
#define MULTI_POLE_TEST_H_

#include <stdbool.h>
#include <stdint.h>

/** Длительность паузы после ожидаемого ALERT, секунд. */
#define MULTI_POLE_TEST_COUNTDOWN_SECONDS (3U)

/** Запустить неблокирующий countdown. */
void multi_pole_test_start_countdown(void);

/**
 * Проверить countdown.
 * @return true после истечения заданного времени.
 */
bool multi_pole_test_countdown_finished(void);

/** Сколько секунд осталось (округление вверх), 0 после завершения. */
uint8_t multi_pole_test_countdown_remaining(void);

#endif /* MULTI_POLE_TEST_H_ */
