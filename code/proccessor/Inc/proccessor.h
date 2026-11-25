#ifndef PROCCESSOR_H_
#define PROCCESSOR_H_

#include "commutator.h"
#include "critical_section.h"

#include <stdbool.h>

/** @brief Сколько шагов укладывается в диапазон MiV..MaV. */
#define GENERATION_MAX_STEPS (10U)

typedef enum {
    Idle_Setting, 
    Commutation, 
    MultiPoleWaitAlert,
    MultiPoleCountdown,
    Generation, 
    TestPassed, 
    TestFailed, 
    TestSequenceComplete,
    HardwareFail,
    State_Count
} State;

/**
 * @brief События, по которым автомат меняет состояние.
 * @details Кто поднимает:
 *          EnterAccepted               - экраны (screens.c) по нажатию Enter;
 *          CommutationComplete         - сам процессор после коммутации;
 *          DeviceCurrent*              - модуль вычислений (calc.c), и только
 *                                        после выхода регулятора на уставку;
 *          TestResultRecorded,
 *          TestSequenceFinished,
 *          ReturnToIdle                - сам процессор;
 *          AlertActive                 - обработчик TIM1_BRK по аппаратной
 *                                        аварии от ALERT INA239.
 */
typedef enum {
    EnterAccepted, 
    TestConfirmedByUser,
    CommutationComplete, 
    DeviceVoltageUnderVoltage,
    DeviceVoltageOverVoltage,
    DeviceCurrentWithinTheRange, 
    DeviceCurrentUnderCurrent,
    DeviceCurrentOverCurrent,
    DeviceSetpointExhausted,
    DeviceVoltageSettleTimeout,
    TestResultRecorded,
    TestSequenceFinished,
    ReturnToIdle,
    TestDeniedByUser,
    OutputData,
    DataOutputed,
    AlertActive,
    CountdownComplete,
    Event_Count
} Event;

typedef enum {
    NoFail,
    UnderCurrent = (1UL << 0), 
    UnderVoltage = (1UL << 1), 
    OverCurrent = (1UL << 2), 
    OverVoltage = (1UL << 3), 
    MathOverflow = (1UL << 4),
    MemoryError = (1UL << 5),
    TypeFail_Count = 7
} TypeFail;

typedef struct {
    void (*on_enter) (void);
    void (*on_exit) (void);
    void (*proccess) (void);
    State (*handle_event) (Event);
} FiniteStateMachine;

extern FiniteStateMachine proccessor[State_Count];
extern volatile Event current_event;
extern volatile TypeFail typefail;
// extern uint8_t generation_step;



void proccessor_init_state (void);
State proccessor_get_state (void);
void proccessor_change_state (State);
Commutator_ActiveLine proccessor_line_from_pin (uint8_t pin);
bool proccessor_more_tests_left (void);
bool proccessor_setup_lines (void);
// bool proccessor_apply_setpoint (void);
void proccessor_raise_after_result (void);
void proccessor_shutdown_generation (void);
void proccessor_force_shutdown_generation (void);
void proccessor_wait_cap_discharge (void);

/**
 * @brief Поднять событие, если ни одно ещё не ждёт разбора.
 * @return true, если событие принято.
 *
 * @details Проверка и запись неделимы относительно прерываний. Иначе
 *          TIM1_BRK_IRQHandler, поднимающий AlertActive, мог вклиниться между
 *          ними, и главный цикл затёр бы аварию своим событием. Это защитный
 *          контур установки с высоким напряжением, терять его сообщения
 *          нельзя. Запрет прерываний здесь - это порядка десятка тактов.
 */
static inline bool proccessor_raise_event (Event event) {
    enter_critsec();
    if (current_event == Event_Count) {
        current_event = event;
        exit_critsec();
        return true;
    } else {
        exit_critsec();
        return false;
    }
}

/** @brief Поднять событие, вытеснив то, что ждало разбора. Для аварии. */
static inline void proccessor_force_raise_event (Event event) {
    enter_critsec();
    current_event = event;
    exit_critsec();
    return;
}

/** @brief Запрос выхода из ручного режима, не вытесняя аппаратную аварию. */
static inline void proccessor_request_mode_exit (void) {
    enter_critsec();
    if (current_event != AlertActive) {
        current_event = ReturnToIdle;
    }
    exit_critsec();
    return;
}

/**
 * @brief Забрать событие на разбор, освободив место для следующего.
 * @return Event_Count, если разбирать нечего.
 *
 * @details Главный цикл обязан пользоваться именно этим, а не читать
 *          current_event и гасить его после разбора. on_exit и on_enter
 *          идут миллисекунды (реле), и авария, поднятая за это время,
 *          при отложенном гашении была бы стёрта не глядя.
 */
static inline Event proccessor_take_event (void) {
    enter_critsec();
    Event event = current_event;
    current_event = Event_Count;
    exit_critsec();
    return event;
}

#endif /* PROCCESSOR_H_ */