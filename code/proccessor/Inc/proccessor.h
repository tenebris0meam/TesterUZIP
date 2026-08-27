#ifndef PROCCESSOR_H_
#define PROCCESSOR_H_

#include "stm32f103xb.h"   /* __get_PRIMASK / __disable_irq / __set_PRIMASK */

#include <stdbool.h>

typedef enum {
    Idle_Setting, 
    Commutation, 
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
    CommutationComplete, 
    DeviceCurrentWithinTheRange, 
    DeviceCurrentUnderCurrent,
    DeviceCurrentOverCurrent,
    TestResultRecorded,
    TestSequenceFinished,
    ReturnToIdle,
    AlertActive,
    Event_Count
} Event;

typedef struct {
    void (*on_enter) (void);
    void (*on_exit) (void);
    void (*proccess) (void);
    State (*handle_event) (Event);
} FiniteStateMachine;

extern const FiniteStateMachine proccessor[State_Count];
extern volatile Event current_event;



void proccessor_init_state (void);
State proccessor_get_state (void);
void proccessor_change_state (State);

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
    bool accepted = false;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (current_event == Event_Count) {
        current_event = event;
        accepted = true;
    }
    __set_PRIMASK(primask);
    return accepted;
}

/** @brief Поднять событие, вытеснив то, что ждало разбора. Для аварии. */
static inline void proccessor_force_raise_event (Event event) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    current_event = event;
    __set_PRIMASK(primask);
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
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    Event event = current_event;
    current_event = Event_Count;
    __set_PRIMASK(primask);
    return event;
}



void proccessor_idle_on_enter (void);
void proccessor_commutation_on_enter (void);
void proccessor_generation_on_enter (void);
void proccessor_testpassed_on_enter (void);
void proccessor_testfailed_on_enter (void);
void proccessor_testsequencecomplete_on_enter (void);
void proccessor_hardwarefail_on_enter (void);



void proccessor_idle_on_exit (void);
void proccessor_commutation_on_exit (void);
void proccessor_generation_on_exit (void);
void proccessor_testpassed_on_exit (void);
void proccessor_testfailed_on_exit (void);
void proccessor_testsequencecomplete_on_exit (void);
void proccessor_hardwarefail_on_exit (void);



void proccessor_idle_proccess (void);
void proccessor_commutation_proccess (void);
void proccessor_generation_proccess (void);
void proccessor_testpassed_proccess (void);
void proccessor_testfailed_proccess (void);
void proccessor_testsequencecomplete_proccess (void);
void proccessor_hardwarefail_proccess (void);



State proccessor_idle_handle_event (Event);
State proccessor_commutation_handle_event (Event);
State proccessor_generation_handle_event (Event);
State proccessor_testpassed_handle_event (Event);
State proccessor_testfailed_handle_event (Event);
State proccessor_testsequencecomplete_handle_event (Event);
State proccessor_hardwarefail_handle_event (Event);



#endif /* PROCCESSOR_H_ */