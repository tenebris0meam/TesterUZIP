#include "relay.h"
#include "gpio.h"
#include "delay.h"

#define RELAY_SETTLE_TIME_MS (5)

static bool is_turn_on_rj45_pe_pos = false;
static bool is_turn_on_rj45_pe_neg = false;
static bool is_turn_on_probe_enable = false;
static bool is_turn_on_rj45_enable = false;

static bool relay_resolve (const Relay_TypeDef relay, 
                           const Pin_TypeDef **nEnable, 
                           bool **is_turn_on) {
    switch (relay) {
        case RJ45Shield_Neg: 
            *is_turn_on = &is_turn_on_rj45_pe_neg;
            *nEnable = &RJ_SHIELD_N;
            return true;
        case RJ45Shield_Pos:
            *is_turn_on = &is_turn_on_rj45_pe_pos;
            *nEnable = &RJ_SHIELD_P; 
            return true;
        case ProbeEnable: 
            *is_turn_on = &is_turn_on_probe_enable;
            *nEnable = &PROBE_EN;
            return true;
        case RJ45Enable:
            *is_turn_on = &is_turn_on_rj45_enable;
            *nEnable = &DSUB_EN;
            return true;
        default: 
            return false;
    }
}

bool relay_get_state (const Relay_TypeDef relay) {
    const Pin_TypeDef *nEnable = 0;
    bool *is_turn_on = 0;

    if (!relay_resolve(relay, &nEnable, &is_turn_on)) {
        return false;
    }
    return *is_turn_on;
}

void relay_set_state (const Relay_TypeDef relay, bool new_state) {
    const Pin_TypeDef *nEnable = 0;
    bool *is_turn_on = 0;

    if (!relay_resolve(relay, &nEnable, &is_turn_on)) {
        return;
    }

    if (*is_turn_on != new_state) {
        Pin_Set_State(nEnable, !new_state);
        delay_ms(RELAY_SETTLE_TIME_MS);
        *is_turn_on = new_state;
    }
    return;
}

void relay_force_off (const Relay_TypeDef relay) {
    const Pin_TypeDef *nEnable = 0;
    bool *is_turn_on = 0;

    if (!relay_resolve(relay, &nEnable, &is_turn_on)) {
        return;
    }

    /*
     * Ждать размыкания не будем: команда  реле отдана, а контакты разойдутся
     * сами. Обратного хода у этой команды нет, поэтому состояние помечаем
     * сразу - иначе следующий вызов решит, что реле всё ещё включено.
     */
    Pin_Set_State(nEnable, true);
    *is_turn_on = false;
    return;
}

void relay_force_off_all (void) {
    // Сначала запрещаем выдачу напряжения
    relay_force_off(ProbeEnable);
    relay_force_off(RJ45Enable);
    // Потом остальные реле
    relay_force_off(RJ45Shield_Pos);
    relay_force_off(RJ45Shield_Neg);
    return;
}
