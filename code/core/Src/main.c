#include "main.h"
#include <stdbool.h>

#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_system.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_utils.h"

#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "spi.h"
#include "system_stm32f1xx.h"
#include "tim.h"
#include "delay.h"
#include "buzzer.h"

#include "calc.h"
#include "display.h"
#include "keyboard.h"
#include "ina239_user.h"
#include "configurator.h"
#include "proccessor.h"



static void disable_JTAG (void) {
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_GPIO_AF_Remap_SWJ_NOJTAG();
    return;
}

static void SystemClock_Config (void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) { }

    LL_RCC_HSE_Enable();

    while (LL_RCC_HSE_IsReady() != 1) { }
    
    // Кварц на 8 МГц, после PLL будет: (8/2) * 15 = 60 МГц
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_2, LL_RCC_PLL_MUL_15);
    LL_RCC_PLL_Enable();

    while (LL_RCC_PLL_IsReady() != 1) { }
    // AHB шина 60 МГц
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    // ABP1 шина 30 МГц
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    // APB2 шина 60 МГц
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    /* Устанавливаем частоту ADC1 10 МГц (60/6=10) */
    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSRC_PCLK2_DIV_6);

    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) { }
    LL_SetSystemCoreClock(60000000);
    LL_Init1msTick(SystemCoreClock);

    delay_init(SystemCoreClock);
    return;
}

static bool Hardware_Init (void) {
    if (!GPIO_Init()) {
        return false;
    }
    
    if (!TIM1_Init()) {
        return false;
    }

    if (!TIM3_Init()) {
        return false;
    }

    if (!SPI2_Init()) {
        return false;
    }

    if (!ADC1_Init()) {
        return false;
    }

    if (!DMA1_SPI2_Init()) {
        return false;
    }

    return true;
}

static bool Software_Init () {
    if (!configurator_read_config()) {
        return false;
    }

    if (!INA239_MainInit()) {
        return false;
    }

    if (!display_init()) {
        return false;
    }

    if (!calc_integrator_Init()) {
        return false;
    }

    keyboard_init();

    proccessor_init_state();
    return true;
}



#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmain-return-type"
__attribute__((noreturn)) void main (void) {

    disable_JTAG();
    SystemClock_Config();

    if (!Hardware_Init()) {
        Error_Handler();
    }

    if (!Software_Init()) {
        Error_Handler();
    }

    while (true) {
        State tester_state = proccessor_get_state();
        Event event = proccessor_take_event();
        if (event != Event_Count) {
            State new_state = proccessor[tester_state].handle_event(event);
            if (tester_state != new_state) {
                proccessor[tester_state].on_exit();
                proccessor_change_state(new_state);
                tester_state = new_state;
                proccessor[tester_state].on_enter();
            }
        }
        proccessor[tester_state].proccess();

        keyboard_routine();
        buzzer_routine();
        if (display_get_render()) {
            display_routine();
        }
    }
}
#pragma clang diagnostic pop

__attribute__((noreturn)) void Error_Handler (void) {
    __disable_irq();
    while (true) {
        LL_GPIO_ResetOutputPin(BUZZER_GPIO_Port, BUZZER_GPIO_Pin);
        delay_ms(500);

        LL_GPIO_SetOutputPin(BUZZER_GPIO_Port, BUZZER_GPIO_Pin);
        delay_ms(500);
    }
}