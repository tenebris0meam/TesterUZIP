/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f1xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_it.h"
#include "calc.h"
#include "commutator.h"
#include "ina239_user.h"
#include "proccessor.h"
#include "spi.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_tim.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
	/* USER CODE BEGIN NonMaskableInt_IRQn 0 */

	/* USER CODE END NonMaskableInt_IRQn 0 */
	/* USER CODE BEGIN NonMaskableInt_IRQn 1 */
	while (1)
	{
	}
	/* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
	/* USER CODE BEGIN HardFault_IRQn 0 */

	/* USER CODE END HardFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_HardFault_IRQn 0 */
		/* USER CODE END W1_HardFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
	/* USER CODE BEGIN MemoryManagement_IRQn 0 */

	/* USER CODE END MemoryManagement_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
		/* USER CODE END W1_MemoryManagement_IRQn 0 */
	}
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
	/* USER CODE BEGIN BusFault_IRQn 0 */

	/* USER CODE END BusFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_BusFault_IRQn 0 */
		/* USER CODE END W1_BusFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
	/* USER CODE BEGIN UsageFault_IRQn 0 */

	/* USER CODE END UsageFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_UsageFault_IRQn 0 */
		/* USER CODE END W1_UsageFault_IRQn 0 */
	}
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void)
{
	/* USER CODE BEGIN SVCall_IRQn 0 */

	/* USER CODE END SVCall_IRQn 0 */
	/* USER CODE BEGIN SVCall_IRQn 1 */

	/* USER CODE END SVCall_IRQn 1 */
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
	/* USER CODE BEGIN DebugMonitor_IRQn 0 */

	/* USER CODE END DebugMonitor_IRQn 0 */
	/* USER CODE BEGIN DebugMonitor_IRQn 1 */

	/* USER CODE END DebugMonitor_IRQn 1 */
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void)
{
	/* USER CODE BEGIN PendSV_IRQn 0 */

	/* USER CODE END PendSV_IRQn 0 */
	/* USER CODE BEGIN PendSV_IRQn 1 */

	/* USER CODE END PendSV_IRQn 1 */
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
	/* USER CODE BEGIN SysTick_IRQn 0 */

	/* USER CODE END SysTick_IRQn 0 */
	/* USER CODE BEGIN SysTick_IRQn 1 */

	/* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */
void TIM1_BRK_IRQHandler(void) {
	/*
	 * Сразу запрещаем собственное прерывание. У INA239 включена защёлка
	 * ALERT (INA239_DIAG_ALATCH_Msk), и снимается она только успешным
	 * чтением DIAG_ALRT. Если чтение не удалось - шина занята обменом по
	 * DMA, - линия остаётся активной и обработчик вызывается снова и снова.
	 * Разрешение вернёт TIM1_Start_PWM() при следующем запуске генерации.
	 */
	NVIC_DisableIRQ(TIM1_BRK_IRQn);

	/** Деактивируем схему коммутации. Неблокирующий вариант: обычный
	* commutator_deny_commutation() ждёт срабатывания контактов по 5 мс
	* на каждое реле, а это обработчик приоритета 0. */
	commutator_forcedeny_commutation();

	/** Выставляем состояние "аппаратная ошибка" */
	proccessor_force_raise_event(AlertActive);

	return;
}

void TIM3_IRQHandler (void) {
	/* сбрасываем флаг обновления таймера */
	LL_TIM_ClearFlag_UPDATE(TIM3);

	/*
	 * Шину занял кто-то другой - пропускаем такт и выходим.
	 * Ждать здесь нельзя: освобождают шину обработчики DMA того же
	 * приоритета 1, вытеснить нас они не могут, и ожидание превратилось бы
	 * в зависание до срабатывания тайм-аута. Одно пропущенное измерение из
	 * десяти тысяч в секунду не значит ничего.
	 */
	if (spi2ctrl.capture(INA239) != SPICtrlStatus_Allow) {
		return;
	}
	spi2ctrl.set_signal(&have_new_data);

	spi2ctrl.operate(request_vbus_and_current,
					 request_size, 
					 (uint8_t *)answer_vbus_and_current, 
					 request_size, 
					 1);

	return;
}

void DMA1_Channel4_IRQHandler (void) {
	/* Сбрасываем флаг завершения приёма */
	LL_DMA_ClearFlag_TC4(DMA1);
	/* Сбрасываем флаг прерывания 4-го канала DMA */
    LL_DMA_ClearFlag_GI4(DMA1);

	spi2ctrl.complete();
	return;
}

void DMA1_Channel5_IRQHandler (void) {
	/* Сбрасываем флаг завершения передачи */
	LL_DMA_ClearFlag_TC5(DMA1);
	/* Сбрасываем флаг прерывания 5-го канала DMA */
    LL_DMA_ClearFlag_GI5(DMA1);

	spi2ctrl.complete();
	return;
}

/* user code end 1 */
