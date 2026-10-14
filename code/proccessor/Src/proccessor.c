#include "proccessor.h"
#include "commutator.h"
#include "model.h"
#include "calc.h"
#include "hv_feedback.h"
#include "params.h"
#include "tim.h"

#include "states/idle_setting.h"
#include "states/testpassed.h"
#include "states/testfailed.h"
#include "states/testsequence_complete.h"
#include "states/hardware_fail.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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
 *          TestPassed / TestFailed    результат лёг в test_results
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

/** @brief Номер шага подъёма уставки внутри текущей проверки, 0..GENERATION_MAX_STEPS. */
uint8_t generation_step = 0;

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
    uint8_t done = (uint8_t)(test_results.passed_cnt + test_results.failed_cnt);
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
    const Params_TestSequence_TypeDef *sequence = test_results.current_sequence;
    const Params_TestParam_TypeDef *testparams = test_results.current_test_params;
    const Params_Device_TypeDef *device = test_results.current_device;

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
    commutator_set_active_line(positive, LineType_Positive);
    commutator_set_active_line(negative, LineType_Negative);

    /*
     * Коммутатор отказывает молча: у щупов доступны только линии 1 и 2,
     * у клеммника K1 - с 1 по 5. Поэтому не верим на слово, а спрашиваем,
     * что он в итоге принял.
     */
    return ((commutator_get_active_line(LineType_Positive) == positive) && 
            (commutator_get_active_line(LineType_Negative) == negative));
}

/**
 * @brief Выставить уставку регулятора под текущий номер шага.
 * @return false, если диапазон напряжений прибора исчерпан.
 *
 * @details Шаг 0 - минимальное напряжение теста, шаг GENERATION_MAX_STEPS -
 *          максимальное. Деление на GENERATION_MAX_STEPS в конце, а не в
 *          начале: иначе на узком диапазоне шаг обнулялся бы целочисленным
 *          делением, и последний шаг не попадал бы точно в MaV.
 */
bool proccessor_apply_setpoint (void) {
    const Params_TestParam_TypeDef *testparams = test_results.current_test_params;

    if (testparams == NULL) {
        return false;
    }
    if (generation_step > GENERATION_MAX_STEPS) {
        return false;
    }

    uint32_t max_volt_afterdiv_uV = hv_feedback_volts_to_target_uV(testparams->max_voltage);
    uint32_t min_volt_afterdiv_uV = hv_feedback_volts_to_target_uV(testparams->min_voltage);

    uint32_t span = max_volt_afterdiv_uV - min_volt_afterdiv_uV;
    /* Диапазон нулевой - поднимать уставку некуда, шаг может быть только один */
    if ((span == 0U) && (generation_step > 0U)) {
        return false;
    }

    uint32_t volts = min_volt_afterdiv_uV + (span * generation_step) / GENERATION_MAX_STEPS;
    if (volts > HV_FEEDBACK_BUS_LIMIT_uV) {
        volts = HV_FEEDBACK_BUS_LIMIT_uV;
    }

    calc_integrator_set_target_voltage(volts);
    return true; 
}

/** @brief Записать результат очередной проверки в модель. */
void proccessor_record_result (bool pass) {
    int32_t voltage_uV = 0;
    int32_t current_nA = 0;
    calc_get_test_data(&voltage_uV, &current_nA);

    TestResult_TypeDef result;
    result.sequence_pos = (uint8_t)(test_results.passed_cnt + test_results.failed_cnt);
    result.device_voltage = voltage_uV;
    result.device_current = ((current_nA > 0) ? (uint32_t)current_nA : 0U);
    result.pass = pass;

    model_handle_test_result(&result);
    return;
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
    int32_t voltage_uV = INT32_MAX, current_nA = INT32_MAX;
    do {
        if (!have_new_data) {
            continue;
        }
        have_new_data = false;
        calc_measurement_update();
        calc_get_test_data(&voltage_uV, &current_nA);
    } while ((voltage_uV >= 500000) && (current_nA >= 20000));
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
    model_init();
    generation_step = 0;
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