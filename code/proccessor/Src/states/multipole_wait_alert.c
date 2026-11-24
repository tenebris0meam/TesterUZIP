#include "states/multipole_wait_alert.h"

#include "display.h"
#include "commutator.h"
#include "ina239_user.h"
#include "proccessor.h"
#include "stm32f1xx_ll_tim.h"
#include "tim.h"

void proccessor_multipole_wait_alert_on_enter(void)
{
    /* Экран штатного тестирования остаётся активным. */
    display_allow_render();

    /* После предыдущей генерации TIM1_BRK_IRQn мог быть отключён. */
    NVIC_ClearPendingIRQ(TIM1_BRK_IRQn);
    NVIC_EnableIRQ(TIM1_BRK_IRQn);

    /* Запускаем минимальную генерацию, чтобы замыкание щупов вызвало ALERT. */
    if (INA239_WriteShuntOvervoltageLimit(INA239_GetInstance(), SOVL_2mA) != INA239_OK) {
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    if (INA239_WriteShuntUndervoltageLimit(INA239_GetInstance(), -SOVL_2mA) != INA239_OK) {
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    if (INA239_SetMode(INA239_MODE_CONT_SHUNT_BUS) != INA239_OK) {
        calc_reset_sweep();
        proccessor_shutdown_generation();
        proccessor_raise_event(ReturnToIdle);
        return;
    }
    TIM1_set_CCR_value(CCR_VAL_MULTIPOLE);
    TIM1_Start_PWM();
    return;
}

void proccessor_multipole_wait_alert_on_exit(void)
{
}

void proccessor_multipole_wait_alert_process(void)
{
    /* Ожидание выполняется исключительно через AlertActive. */
}

State proccessor_multipole_wait_alert_handle_event(Event event)
{
    switch (event) {
        case AlertActive:
            /*
             * В этом состоянии любой ALERT считается ожидаемым подтверждением.
             * BKIN уже остановил выходы TIM1 и ISR разомкнул коммутатор.
             * Полностью переинициализируем INA239 до начала countdown.
             */
            TIM1_Stop_PWM();
            TIM1_set_CCR_value(0U);

            if (!INA239_MainInit()) {
                return HardwareFail;
            }

            /* Чтение DIAG_ALRT при MainInit снимает защёлку ALERT. */
            LL_TIM_ClearFlag_BRK(TIM1);
            NVIC_ClearPendingIRQ(TIM1_BRK_IRQn);
            NVIC_EnableIRQ(TIM1_BRK_IRQn);

            /*
             * ALERT ISR принудительно разомкнул реле. Восстанавливаем
             * коммутацию текущей штатной последовательности; countdown
             * даёт реле время завершить переключение до старта генерации.
             */
            if (!proccessor_setup_lines()) {
                return HardwareFail;
            }
            commutator_allow_commutation();

            return MultiPoleCountdown;

        case ReturnToIdle:
            return Idle_Setting;

        default:
            return MultiPoleWaitAlert;
    }
}
