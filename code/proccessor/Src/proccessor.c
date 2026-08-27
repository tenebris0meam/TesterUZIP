#include "proccessor.h"
#include "commutator.h"
#include "display.h"
#include "model.h"
#include "calc.h"
#include "hv_feedback.h"
#include "ina239_user.h"
#include "stm32f1xx_ll_tim.h"
#include "tim.h"
#include "keyboard.h"
#include "buzzer.h"
#include "input_events.h"
#include "delay.h"

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

/** @brief Сколько шагов укладывается в диапазон MiV..MaV. */
#define GENERATION_MAX_STEPS (100U / GENERATION_STEP_PERCENT)

/**
 * @brief Сколько ждать выхода на уставку, мс.
 * @details Худший случай генерации - GENERATION_MAX_STEPS + 1 шагов подряд,
 *          то есть 11 * 2 с = 22 с под напряжением. Дальше автомат уйдёт
 *          в TestFailed при любом исходе.
 */
#define GENERATION_STEP_TIMEOUT_MS (2000U)

/** @brief Номера выводов в конфигурации: 0..8, что соответствует ActiveLine_1..9. */
#define PROCCESSOR_PINS_NUM (9U)



const FiniteStateMachine proccessor[State_Count] = {
    [Idle_Setting] = {
        .on_enter = proccessor_idle_on_enter, 
        .on_exit = proccessor_idle_on_exit, 
        .proccess = proccessor_idle_proccess, 
        .handle_event = proccessor_idle_handle_event
    }, 
    [Commutation] = {
        .on_enter = proccessor_commutation_on_enter,
        .on_exit = proccessor_commutation_on_exit,
        .proccess = proccessor_commutation_proccess,
        .handle_event = proccessor_commutation_handle_event
    },
    [Generation] = {
        .on_enter = proccessor_generation_on_enter, 
        .on_exit = proccessor_generation_on_exit, 
        .proccess = proccessor_generation_proccess, 
        .handle_event = proccessor_generation_handle_event
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
        .on_enter = proccessor_hardwarefail_on_enter, 
        .on_exit = proccessor_hardwarefail_on_exit, 
        .proccess = proccessor_hardwarefail_proccess,
        .handle_event = proccessor_hardwarefail_handle_event
    }
};

static State current_state = State_Count;
volatile Event current_event = Event_Count;

/** @brief Номер шага подъёма уставки внутри текущей проверки, 0..GENERATION_MAX_STEPS. */
static uint8_t generation_step = 0;

/** @brief Идентификатор задачи для сторожевого таймера. */
static int32_t generation_step_start = 0;
static uint8_t generation_delay_task_id = 0;

/* test_config и test_results определены в model.c вместе с операциями над
 * ними - см. code/proccessor/Src/model.c */



/* ------------------------------------------------------------------------ */
/*                          Вспомогательные функции                          */
/* ------------------------------------------------------------------------ */

/**
 * @brief Номер вывода из конфигурации -> линия коммутатора.
 * @details В конфигурации выводы нумеруются с нуля, а ноль у коммутатора
 *          занят под ActiveLine_None.
 */
static Commutator_ActiveLine proccessor_line_from_pin (uint8_t pin) {
    if (pin >= PROCCESSOR_PINS_NUM) {
        return ActiveLine_None;
    }
    return (Commutator_ActiveLine)(pin + 1U);
}

/** @brief Есть ли ещё невыполненные проверки у выбранного прибора. */
static bool proccessor_more_tests_left (void) {
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
static bool proccessor_setup_lines (void) {
    const Params_TestSequence_TypeDef *sequence = test_results.current_sequence;
    const Params_Device_TypeDef *device = test_results.current_device;

    if ((sequence == NULL) || (device == NULL)) {
        return false;
    }

    /* Прибор должен поддерживать выбранный оператором разъём */
    if ((device->connector_types & (uint8_t)test_config.interface_id) == 0U) {
        return false;
    }

    /* Диапазон напряжений теста должен быть осмысленным и достижимым */
    if ((device->minimal_voltage_test == 0U) || 
        (device->maximal_voltage_test < device->minimal_voltage_test) || 
        (device->maximal_voltage_test > HV_FEEDBACK_MAX_TEST_V)) {
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

    commutator_set_active_interface(test_config.interface_id);
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
static bool proccessor_apply_setpoint (void) {
    const Params_Device_TypeDef *device = test_results.current_device;

    if (device == NULL) {
        return false;
    }
    if (generation_step > GENERATION_MAX_STEPS) {
        return false;
    }

    uint32_t span = (uint32_t)(device->maximal_voltage_test - 
                               device->minimal_voltage_test);
    /* Диапазон нулевой - поднимать уставку некуда, шаг может быть только один */
    if ((span == 0U) && (generation_step > 0U)) {
        return false;
    }

    uint32_t volts = (uint32_t)device->minimal_voltage_test + 
                     ((span * generation_step) / GENERATION_MAX_STEPS);
    if (volts > HV_FEEDBACK_MAX_TEST_V) {
        volts = HV_FEEDBACK_MAX_TEST_V;
    }

    calc_integrator_set_target_voltage(hv_feedback_volts_to_target_uV((uint16_t)volts));
    return delay_register_task(&generation_delay_task_id, GENERATION_STEP_TIMEOUT_MS, Millis);
}

/** @brief Записать результат очередной проверки в модель. */
static void proccessor_record_result (bool pass) {
    int32_t voltage_uV = 0;
    int32_t current_nA = 0;
    calc_get_test_data(&voltage_uV, &current_nA);

    TestResult_TypeDef result;
    result.sequence_pos = (uint8_t)(test_results.passed_cnt + test_results.failed_cnt);
    result.device_voltage = hv_feedback_uV_to_volts((voltage_uV > 0) ? 
                                                    (uint32_t)voltage_uV : 0U);
    result.device_current = ((current_nA > 0) ? (uint32_t)current_nA : 0U);
    result.pass = pass;

    model_handle_test_result(&result);
    return;
}

/** @brief Что делать после записи результата: следующая проверка или итог. */
static void proccessor_raise_after_result (void) {
    proccessor_raise_event(proccessor_more_tests_left() ? 
                           TestResultRecorded : 
                           TestSequenceFinished);
    return;
}

/** @brief Обесточить генерацию. Порядок: сначала возбуждение, потом цепь. */
static void proccessor_shutdown_generation (void) {
    TIM1_Stop_PWM();
    TIM1_set_CCR_value(0);
    TIM3_Stop_Count();
    return;
}



/* ------------------------------------------------------------------------ */
/*                            Служебные функции                              */
/* ------------------------------------------------------------------------ */

void proccessor_init_state (void) {
    model_init();
    generation_step = 0;
    generation_step_start = 0;
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

/* ------------------------------------------------------------------------ */
/*                            Idle/Setting                                  */
/* ------------------------------------------------------------------------ */

void proccessor_idle_on_enter (void) {
    return; 
}

void proccessor_idle_on_exit (void) {
    /*
     * Выйти из Idle можно только на запуск прогона, поэтому результаты
     * прошлого прогона обнуляем здесь - и счётчики, и указатель на
     * очередную последовательность.
     */
    model_reset_results();
    generation_step = 0;
    return;
}

void proccessor_idle_proccess (void) {
    keyboard_routine();
    buzzer_routine();
    display_routine();
    return;
}

State proccessor_idle_handle_event (Event event) {
    switch (event) {
        case EnterAccepted: 
            if (display_get_current_screen() == ScreenRunningTest) {
                return Commutation;
            }
            return Idle_Setting;
        case AlertActive: return HardwareFail;
        default: return Idle_Setting;
    }
}

/* ------------------------------------------------------------------------ */
/*                            Commutation                                   */
/* ------------------------------------------------------------------------ */

void proccessor_commutation_on_enter (void) {
    return;
}

void proccessor_commutation_on_exit (void) {
    return;
}

void proccessor_commutation_proccess (void) {
    if (!proccessor_more_tests_left() || (test_results.current_sequence == NULL)) {
        proccessor_raise_event(TestSequenceFinished);
        return;
    }

    /* Была нажата кнопка ОК */
    buzzer_routine();

    /*
     * Единственное место, где экран перерисовывается во время прогона.
     * Во время генерации этого делать не стоит: шину SPI2 в этот момент
     * каждые 100 мкс занимает INA239, а показывать нечего - на экране
     * прогона меняются только номера выводов и счётчик проверок, то есть
     * ровно то, что обновляется здесь, между проверками.
     */
    display_routine();

    if (!proccessor_setup_lines()) {
        /*
         * Проверку выполнить нечем. Засчитываем провал и НЕ подаём напряжение;
         * следующий проход возьмёт следующую позицию, а когда они кончатся -
         * поднимет TestSequenceFinished ветвью выше.
         */
        proccessor_record_result(false);
        return;
    }

    commutator_allow_commutation();
    proccessor_raise_event(CommutationComplete);
    return;
}

State proccessor_commutation_handle_event (Event event) {
    switch (event) {
        case CommutationComplete: return Generation;
        case TestSequenceFinished: return TestSequenceComplete;
        case AlertActive: return HardwareFail;
        default: return Commutation;
    }
}

/* ------------------------------------------------------------------------ */
/*                             Generation                                   */
/* ------------------------------------------------------------------------ */

void proccessor_generation_on_enter (void) {
    calc_reset_integrator();
    calc_integrator_Init();
    generation_step = 0;

    /*
     * Скважность обнуляем ДО включения выходов: в CCR лежит остаток от
     * предыдущей проверки, и без этого генератор рванул бы сразу на него.
     */
    TIM1_set_CCR_value(0);

    /* На шаге 0 уставка есть всегда: диапазон проверен в proccessor_setup_lines() */
    (void)proccessor_apply_setpoint();

    INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS);
    TIM3_Start_Count();
    TIM1_Start_PWM();
    return;
}

void proccessor_generation_on_exit (void) {
    proccessor_shutdown_generation();
    INA239_SetMode(INA239_MODE_SHUTDOWN);
    commutator_deny_commutation();
    /*
     * Интегратор здесь НЕ сбрасываем, хотя напрашивается. Главный цикл
     * вызывает on_exit прежнего состояния до on_enter нового, а нового -
     * TestPassed или TestFailed - первым делом читает calc_get_test_data().
     * Сброс на этом месте обнулял бы напряжение и ток ровно перед тем, как
     * их запишут, и в протокол ложились бы нули. Сбрасывает тот, кто
     * начинает следующую генерацию, - proccessor_generation_on_enter().
     */
    return;
}

void proccessor_generation_proccess (void) {
    if (have_new_data) {
        have_new_data = false;
        calc_routine();

        int32_t duty_ppm = calc_integrator_get_integral_value();
        if (duty_ppm < 0) {
            duty_ppm = 0;
        }
        if (duty_ppm > CALC_DUTY_MAX_PPM) {
            duty_ppm = CALC_DUTY_MAX_PPM;
        }

        uint32_t new_CCR1_val = 
            (uint32_t)(((uint64_t)LL_TIM_GetAutoReload(TIM1) * (uint32_t)duty_ppm) / 
                       1000000ULL);
        if (new_CCR1_val != LL_TIM_OC_GetCompareCH1(TIM1)) {
            TIM1_set_CCR_value(new_CCR1_val);
        }
    }

    /*
     * Сторожевой таймер шага. Вердикт по току выносится только после выхода
     * на уставку, поэтому отказ, при котором на уставку выйти нельзя
     * (пробитый прибор держит напряжение у нуля), не породил бы ни одного
     * события - и высокое напряжение осталось бы на выходе навсегда.
     * Такой исход и по смыслу превышение тока: прибор проводит больше, чем
     * генератор может продавить.
     */
    if (!calc_integrator_is_settled() && 
        !delay_is_not_yet_expired(generation_delay_task_id)) {
        proccessor_raise_event(DeviceCurrentOverCurrent);
    }

    buzzer_routine();
    return;
}

State proccessor_generation_handle_event (Event event) {
    switch (event) {
        case DeviceCurrentWithinTheRange: return TestPassed;
        case DeviceCurrentOverCurrent: return TestFailed;
        case DeviceCurrentUnderCurrent: 
            /*
             * Ток ниже нормы - прибор ещё не открылся. Поднимаем уставку на
             * шаг (10 % ширины диапазона из конфига) и остаёмся здесь же.
             * Смена уставки сбрасывает признак выхода на неё, так что
             * следующий вердикт придёт только после нового устоявшегося
             * измерения. Кончился диапазон - прибор не открылся на всём
             * рабочем напряжении, это провал.
             */
            generation_step++;
            if (!proccessor_apply_setpoint()) {
                return TestFailed;
            }
            return Generation;
        case AlertActive: return HardwareFail;
        default: return Generation;
    }
}

/* ------------------------------------------------------------------------ */
/*                             TestPassed                                   */
/* ------------------------------------------------------------------------ */

void proccessor_testpassed_on_enter (void) {
    proccessor_record_result(true);
    return;
}

void proccessor_testpassed_on_exit (void) {
    return;
}

void proccessor_testpassed_proccess (void) {
    proccessor_raise_after_result();
    return;
}

State proccessor_testpassed_handle_event (Event event) {
    switch (event) {
        case TestResultRecorded: return Commutation;
        case TestSequenceFinished: return TestSequenceComplete;
        case AlertActive: return HardwareFail;
        default: return TestPassed;
    }
}

/* ------------------------------------------------------------------------ */
/*                             TestFailed                                   */
/* ------------------------------------------------------------------------ */

void proccessor_testfailed_on_enter (void) {
    proccessor_record_result(false);
    return;
}

void proccessor_testfailed_on_exit (void) {
    return;
}

void proccessor_testfailed_proccess (void) {
    proccessor_raise_after_result();
    return;
}

State proccessor_testfailed_handle_event (Event event) {
    switch (event) {
        case TestResultRecorded: return Commutation;
        case TestSequenceFinished: return TestSequenceComplete;
        case AlertActive: return HardwareFail;
        default: return TestFailed;
    }
}

/* ------------------------------------------------------------------------ */
/*                          TestSequenceComplete                            */
/* ------------------------------------------------------------------------ */

void proccessor_testsequencecomplete_on_enter (void) {
    /* Экран сам перейдёт на итоги: событие увидит inputevent_routine() */
    inputevent_testing_completed();
    buzzer_turn_on();
    return;
}

void proccessor_testsequencecomplete_on_exit (void) {
    return;
}

void proccessor_testsequencecomplete_proccess (void) {
    proccessor_raise_event(ReturnToIdle);
    return;
}

State proccessor_testsequencecomplete_handle_event (Event event) {
    switch (event) {
        case ReturnToIdle: return Idle_Setting;
        case AlertActive: return HardwareFail;
        default: return TestSequenceComplete;
    }
}

/* ------------------------------------------------------------------------ */
/*                              HardwareFail                                */
/* ------------------------------------------------------------------------ */

void proccessor_hardwarefail_on_enter (void) {
    /*
     * Сюда попадаем и по AlertActive из TIM1_BRK, и из состояний, где
     * генерации нет. Поэтому обесточиваем всё безусловно, не полагаясь на
     * то, что это уже сделал обработчик прерывания.
     */
    proccessor_shutdown_generation();
    commutator_forcedeny_commutation();
    calc_reset_integrator();

    buzzer_turn_on();
    display_set_screen(ScreenHardwareFail);
    return;
}

void proccessor_hardwarefail_on_exit (void) {
    return;
}

void proccessor_hardwarefail_proccess (void) {
    /*
     * Генерации нет, поэтому крутим то же, что и в Idle: оператор должен
     * увидеть экран аварии и подтвердить его кнопкой.
     */
    keyboard_routine();
    buzzer_routine();
    display_routine();
    return;
}

State proccessor_hardwarefail_handle_event (Event event) {
    switch (event) {
        case EnterAccepted: return Idle_Setting;
        default: return HardwareFail;
    }
}