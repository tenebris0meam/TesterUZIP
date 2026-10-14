#include "stm32f103xb.h"
#include "delay.h"

#define DELAY_MAX_TASKS (32)

typedef struct {
    uint32_t register_time;
    uint32_t requested_delay;
    bool is_expired;
} DelayTask;

static volatile DelayTask tasks[DELAY_MAX_TASKS] = { 0 };

static uint32_t coeff_us2tick = 0;
static uint32_t coeff_ms2tick = 0;

void delay_init (uint32_t SystemCoreClock_Hz) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // разрешаем трассировку
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // разрешаем счёт тактов

    coeff_us2tick = SystemCoreClock_Hz / 1000000;
    coeff_ms2tick = SystemCoreClock_Hz / 1000;

    for (uint8_t i = 0; i < DELAY_MAX_TASKS; i++) {
        tasks[i].is_expired = true;
    }
    return;
}

bool delay_register_task (uint8_t *task_id, uint32_t delay, DelayTimeType type_delay) {
    switch (type_delay) {
        case Millis: 
            if (delay >= 71583) {
                return false;
            }
            break;
        case Micros: 
            if (delay >= 71582789) {
                return false;
            }
            break;
    }
    
    for (uint8_t free_task = 0; free_task < DELAY_MAX_TASKS; free_task++) {
        if (tasks[free_task].is_expired) {
            tasks[free_task].is_expired = false;
            tasks[free_task].register_time = DWT->CYCCNT;
            tasks[free_task].requested_delay = (type_delay == Millis) ? 
                                                delay * coeff_ms2tick : 
                                                delay * coeff_us2tick;
            *task_id = free_task;
            return true;
        }
    }
    return false;
}

void delay_update_task (uint8_t task_id) {
    tasks[task_id].register_time = DWT->CYCCNT;
    return;
}

void delay_unregister_task (uint8_t task_id) {
    if (task_id >= DELAY_MAX_TASKS) {
        return;
    }
    tasks[task_id].is_expired = true;
    return;
}

bool delay_is_not_yet_expired (uint8_t task_id) {
    if (task_id >= DELAY_MAX_TASKS) {
        return false;
    }

    uint32_t cur_time = DWT->CYCCNT;
    if (tasks[task_id].register_time >= cur_time) {
        if (UINT32_MAX - tasks[task_id].register_time + cur_time < tasks[task_id].requested_delay) {
            return true;
        } else {
            tasks[task_id].is_expired = true;
            return false;
        }
    } else {
        if (cur_time - tasks[task_id].register_time < tasks[task_id].requested_delay) {
            return true;
        } else {
            tasks[task_id].is_expired = true;
            return false;
        }
    }
}

void delay_ms (uint32_t ms) {
    uint8_t delay_id = 0;
    if (delay_register_task(&delay_id, ms, Millis)) {
        while (delay_is_not_yet_expired(delay_id)) {}
    }
    return;
}

void delay_us (uint32_t us) {
    uint8_t delay_id = 0;
    if (delay_register_task(&delay_id, us, Micros)) {
        while (delay_is_not_yet_expired(delay_id)) {}
    }
    return;
}