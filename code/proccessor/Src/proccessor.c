#include "proccessor.h"
#include "commutation.h"
#include "commutator.h"
#include "generation.h"
#include "model.h"
#include "calc.h"
#include "hv_feedback.h"
#include "params.h"
#include "system_stm32f1xx.h"
#include "tim.h"

#include "states/idle_setting.h"
#include "states/testpassed.h"
#include "states/testfailed.h"
#include "states/testsequence_complete.h"
#include "states/hardware_fail.h"
#include "states/multipole_wait_alert.h"
#include "states/multipole_countdown.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**
 * @file proccessor.c
 * @brief Автомат проведения проверки прибора.
 *
 * @details Ход одной проверки:
 *
 *          Idle_Setting  оператор набрал прибор и разъём, нажал Enter
 *                |
 *          Commutation   выставили линии очередной последовательности
 *                |       и замкнули коммутацию
 *          Generation    уставка = минимальное напряжение теста из конфига;
 *                |       регулятор выходит на неё и только ПОСЛЕ этого
 *                |       модуль вычислений судит о токе:
 *                |         ток мал   -> уставка +10 % ширины диапазона,
 *                |                      остаёмся здесь же;
 *                |         ток в норме -> проверка пройдена;
 *                |         ток велик   -> проверка провалена
 *                |
 *          TestPassed / TestFailed    результат лёг в test_runtime
 *                |
 *                +-- проверки ещё остались -> Commutation
 *                +-- проверок больше нет   -> TestSequenceComplete -> Idle
 *
 *          Из любого состояния по AlertActive (аппаратная авария от ALERT
 *          INA239, приходит из TIM1_BRK_IRQHandler) уходим в HardwareFail.
 *
 *          ПРО БЕЗОПАСНОСТЬ. Это установка с высоким напряжением, поэтому
 *          порядок действий здесь не косметика:
 *            * скважность обнуляется ДО включения выходов TIM1;
 *            * выходы TIM1 гасятся ДО размыкания реле;
 *            * ни одна проверка не начинается, пока коммутатор не подтвердил,
 *              что принял обе линии;
 *            * у каждого шага уставки есть сторожевой таймер: если регулятор
 *              не может выйти на уставку, вердикт по току не выносится
 *              никогда, и без сторожа напряжение осталось бы на выходе.
 */

/** @brief Шаг подъёма уставки, процентов от ширины диапазона из конфига. */
#define GENERATION_STEP_PERCENT (10U)

/** @brief Номера выводов в конфигурации: 0..8, что соответствует ActiveLine_1..9. */
#define PROCCESSOR_PINS_NUM (9U)



FiniteStateMachine proccessor[State_Count] = {
    [Idle_Setting] = {
        .on_enter = proccessor_idle_on_enter, 
        .on_exit = proccessor_idle_on_exit, 
        .proccess = proccessor_idle_proccess, 
        .handle_event = proccessor_idle_handle_event
    }, 
#ifdef TESTER_MODES
    [Commutation] = {
        .on_enter = 0,
        .on_exit = 0,
        .proccess = 0,
        .handle_event = 0
    },
    [Generation] = {
        .on_enter = 0, 
        .on_exit = 0, 
        .proccess = 0, 
        .handle_event = 0
    },
#else
    [Commutation] = {
        .on_enter = proccessor_commutation_on_enter_checkdevice,
        .on_exit = proccessor_commutation_on_exit,
        .proccess = proccessor_commutation_proccess_default,
        .handle_event = proccessor_commutation_handle_event_default
    },
    [Generation] = {
        .on_enter = proccessor_generation_on_enter_checkdevice, 
        .on_exit = proccessor_generation_on_exit_checkdevice_default, 
        .proccess = proccessor_generation_proccess_checkdevice_default, 
        .handle_event = proccessor_generation_handle_event_checkdevice_default
    },
#endif
    [MultiPoleWaitAlert] = {
        .on_enter = proccessor_multipole_wait_alert_on_enter,
        .on_exit = proccessor_multipole_wait_alert_on_exit,
        .proccess = proccessor_multipole_wait_alert_process,
        .handle_event = proccessor_multipole_wait_alert_handle_event
    },
    [MultiPoleCountdown] = {
        .on_enter = proccessor_multipole_countdown_on_enter,
        .on_exit = proccessor_multipole_countdown_on_exit,
        .proccess = proccessor_multipole_countdown_process,
        .handle_event = proccessor_multipole_countdown_handle_event
    },
    [TestPassed] = {
        .on_enter = proccessor_testpassed_on_enter, 
        .on_exit = proccessor_testpassed_on_exit, 
        .proccess = proccessor_testpassed_proccess, 
        .handle_event = proccessor_testpassed_handle_event
    }, 
    [TestFailed] = {
        .on_enter = proccessor_testfailed_on_enter, 
        .on_exit = proccessor_testfailed_on_exit, 
        .proccess = proccessor_testfailed_proccess,
        .handle_event = proccessor_testfailed_handle_event
    }, 
    [TestSequenceComplete] = {
        .on_enter = proccessor_testsequencecomplete_on_enter, 
        .on_exit = proccessor_testsequencecomplete_on_exit,
        .proccess = proccessor_testsequencecomplete_proccess,
        .handle_event = proccessor_testsequencecomplete_handle_event
    }, 
    [HardwareFail] = {
        .on_enter = proccessor_hardwarefail_on_enter_default, 
        .on_exit = proccessor_hardwarefail_on_exit_default, 
        .proccess = proccessor_hardwarefail_proccess_default,
        .handle_event = proccessor_hardwarefail_handle_event_default
    }
};

static State current_state = State_Count;
volatile Event current_event = Event_Count;

/** @brief Тип аппаратной ошибки */
volatile TypeFail typefail = NoFail;


/**
 * @brief Номер вывода из конфигурации -> линия коммутатора.
 * @details В конфигурации выводы нумеруются с нуля, а ноль у коммутатора
 *          занят под ActiveLine_None.
 */
Commutator_ActiveLine proccessor_line_from_pin (uint8_t pin) {
    if (pin >= PROCCESSOR_PINS_NUM) {
        return ActiveLine_None;
    }
    return (Commutator_ActiveLine)(pin + 1U);
}

/** @brief Есть ли ещё невыполненные проверки у выбранного прибора. */
bool proccessor_more_tests_left (void) {
    uint8_t done = (uint8_t)(test_runtime.passed_cnt + test_runtime.failed_cnt);
    return (done < model_get_total_tests());
}

/**
 * @brief Подготовить коммутацию под очередную проверку.
 * @return true, если коммутатор принял обе линии и прибор можно подключать.
 *
 * @details Возврат false означает "эту проверку выполнить нельзя": разъём не
 *          тот, вывод вне диапазона, диапазон напряжений в конфигурации
 *          неисправен. Под напряжение в таком случае не идём вообще - проверка
 *          сразу засчитывается как проваленная. 
 */
bool proccessor_setup_lines (void) {
    const Params_TestSequence_TypeDef *sequence = test_runtime.current_sequence;
    const Params_TestParam_TypeDef *testparams = test_runtime.current_test_params;
    const Params_Device_TypeDef *device = test_runtime.current_device;

    if ((sequence == NULL) || 
        (device == NULL) || 
        (testparams == NULL)) {
        return false;
    }

    /* Диапазон напряжений теста должен быть осмысленным и достижимым */
    if ((testparams->min_voltage == 0U) || 
        (testparams->max_voltage < testparams->min_voltage) || 
        (testparams->max_voltage > HV_FEEDBACK_MAX_TEST_V)) {
        return false;
    }

    Commutator_ActiveLine positive = proccessor_line_from_pin(sequence->input_pin);
    Commutator_ActiveLine negative = proccessor_line_from_pin(sequence->output_pin);

    if ((positive == ActiveLine_None) || (negative == ActiveLine_None)) {
        return false;
    }
    /* Одна и та же линия на оба полюса - это короткое замыкание генератора */
    if (positive == negative) {
        return false;
    }

    commutator_set_active_interface(device->connector_id);
    if (device->connector_id == Probe) {
        positive = ActiveLine_1;
        negative = ActiveLine_2;
    }
    commutator_set_active_line(positive, LineType_Positive);
    commutator_set_active_line(negative, LineType_Negative);
    return ((commutator_get_active_line(LineType_Positive) == positive) && 
            (commutator_get_active_line(LineType_Negative) == negative));
}

/** @brief Что делать после записи результата: следующая проверка или итог. */
void proccessor_raise_after_result (void) {
    proccessor_raise_event(proccessor_more_tests_left() ? 
                           TestResultRecorded : 
                           TestSequenceFinished);
    return;
}

/** @brief Дождаться разрядки конденсатора генератора */
void proccessor_wait_cap_discharge (void) {
    uint32_t cur_CYCCNT = DWT->CYCCNT;
    uint32_t timeout_tick = cur_CYCCNT + SystemCoreClock;
    const CalcMetrics_TypeDef *metrics;
    do {
        if (DWT->CYCCNT > timeout_tick) {
            break;
        }
        if (!have_new_data) {
            continue;
        }
        have_new_data = false;
        calc_measurement_update();
        metrics = calc_get_current_metrics();
    } while ((metrics->voltage_uV >= 500000) || (metrics->current_nA >= 20000));
    return;
}

/** @brief Обесточить генерацию. Порядок: сначала возбуждение, потом цепь. */
void proccessor_shutdown_generation (void) {
    TIM1_Stop_PWM();
    TIM1_set_CCR_value(0);
    proccessor_wait_cap_discharge();
    TIM3_Stop_Count();
    return;
}

void proccessor_force_shutdown_generation (void) {
    TIM1_Stop_PWM();
    TIM1_set_CCR_value(0);
    return;
}

void proccessor_init_state (void) {
    calc_reset_sweep();
    current_state = Idle_Setting;
    return;
}

State proccessor_get_state (void) {
    return current_state;
}

void proccessor_change_state (State new_state) {
    current_state = new_state;
    return;
}