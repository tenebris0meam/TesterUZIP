#ifndef RELAY_H_
#define RELAY_H_

#include <stdbool.h>

typedef enum {
    RJ45Shield_Neg,
    RJ45Shield_Pos,
    ProbeEnable,
    RJ45Enable,
    Relay_Count
} Relay_TypeDef;

bool relay_get_state (const Relay_TypeDef relay);

/**
 * @brief Переключить реле и выждать время срабатывания.
 * @warning Содержит delay_ms(RELAY_SETTLE_TIME_MS). Из обработчика прерывания
 *          вызывать нельзя - для аварийного пути есть relay_force_off().
 */
void relay_set_state (const Relay_TypeDef relay, bool new_state);

/**
 * @brief Обесточить реле немедленно, без ожидания.
 * @details Для аварийного пути: команда на реле уходит сразу, а время
 *          физического размыкания контактов процессор не ждёт - ему в этот
 *          момент есть чем заняться. Состояние в учёте помечается сразу,
 *          потому что команда уже отдана и обратного хода нет.
 */
void relay_force_off (const Relay_TypeDef relay);

/** @brief Обесточить все реле немедленно. */
void relay_force_off_all (void);

#endif /* RELAY_H_ */
