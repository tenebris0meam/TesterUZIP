#include "delay.h"
#include "gpio.h"
#include "spi.h"
#include "dma.h"
#include "main.h"
#include "stm32f103xb.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_bus.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Неизменяемая часть настроек SPI2.
 * @details Лежит во Flash. Меняются только полярность, фаза и направление -
 *          их накладываем поверх шаблона на стеке.
 */
static const LL_SPI_InitTypeDef SPI2_InitTemplate = {
    .TransferDirection = LL_SPI_FULL_DUPLEX,
    .Mode = LL_SPI_MODE_MASTER,
    .DataWidth = LL_SPI_DATAWIDTH_8BIT,
    .ClockPolarity = LL_SPI_POLARITY_LOW,
    .ClockPhase = LL_SPI_PHASE_2EDGE,
    .NSS = LL_SPI_NSS_SOFT,
    .BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV8,
    .BitOrder = LL_SPI_MSB_FIRST,
    .CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE,
    .CRCPoly = 10
};

/**
 * @brief Настройки, в которых шина стоит между транзакциями.
 * @details Это настройки INA239: синхронный обмен с ним идёт без DMA и
 *          рассчитывает застать шину готовой.
 */
#define SPI2_DEFAULT_MODE (SPI01)
#define SPI2_DEFAULT_DIRECTION (SPI_FullDuplex)

static uint8_t current_mode = (uint8_t)SPI2_DEFAULT_MODE;
static uint8_t current_direction = (uint8_t)SPI2_DEFAULT_DIRECTION;

static volatile bool captured = false;
static volatile SPIDevices holded_device = INA239;
static volatile bool *signal = NULL;

/** @brief DMA закончил перекладывать байты. Сдвиговый регистр ещё может быть занят. */
static volatile bool xfer_done = false;

/** @brief Кто доводит текущий обмен до конца. */
static volatile SPI2_Release_Policy release_policy = SPI2_ReleaseInIsr;

/**
 * @brief Ограничитель на ожидание сдвигового регистра.
 * @details Один байт при SCK 3.75 МГц уходит примерно за 2.2 мкс, то есть
 *          около 130 тактов ядра. Берём с большим запасом, но конечным: в
 *          прошивке не должно остаться ни одного цикла без выхода.
 */
#define SPI2_BSY_GUARD (4000U)



/**
 * @brief Применить режим и направление.
 */
static void spi2_apply_config (SPI_Modes mode, SPI_TransferDirection direction) {
    if ((current_mode == (uint8_t)mode) && 
        (current_direction == (uint8_t)direction) && 
        LL_SPI_IsEnabled(SPI2)) {
        return;
    }

    current_mode = (uint8_t)mode;
    current_direction = (uint8_t)direction;

    LL_SPI_DeInit(SPI2);

    LL_SPI_InitTypeDef init = SPI2_InitTemplate;
    switch (mode) {
        case SPI00:
            init.ClockPolarity = LL_SPI_POLARITY_LOW;
            init.ClockPhase = LL_SPI_PHASE_1EDGE;
            break;
        case SPI01:
            init.ClockPolarity = LL_SPI_POLARITY_LOW;
            init.ClockPhase = LL_SPI_PHASE_2EDGE;
            break;
        case SPI10:
            init.ClockPolarity = LL_SPI_POLARITY_HIGH;
            init.ClockPhase = LL_SPI_PHASE_1EDGE;
            break;
        case SPI11:
            init.ClockPolarity = LL_SPI_POLARITY_HIGH;
            init.ClockPhase = LL_SPI_PHASE_2EDGE;
            break;
    }
    switch (direction) {
        case SPI_FullDuplex: init.TransferDirection = LL_SPI_FULL_DUPLEX; break;
        case SPI_HalfDuplex_RX: init.TransferDirection = LL_SPI_HALF_DUPLEX_RX; break;
        case SPI_HalfDuplex_TX: init.TransferDirection = LL_SPI_HALF_DUPLEX_TX; break;
    }

    if (LL_SPI_Init(SPI2, &init) == SUCCESS) {
        LL_SPI_Enable(SPI2);
    }
    return;
}

/** @brief Настроить шину под конкретное устройство. */
static void spi2_apply_device_config (SPIDevices device) {
    switch (device) {
        case LCD:
            spi2_apply_config(SPI00, SPI_HalfDuplex_TX);
            break;
        case INA239:
        default:
            spi2_apply_config(SPI2_DEFAULT_MODE, SPI2_DEFAULT_DIRECTION);
            break;
    }
    return;
}

/**
 * @brief Вычистить приёмник.
 * @details Байт, оставшийся от прерванного обмена, сдвинул бы следующий кадр:
 *          calc_get_raw_val() разбирает ответ INA239 по фиксированным индексам.
 */
static void spi2_drain_rx (void) {
    while (LL_SPI_IsActiveFlag_RXNE(SPI2)) {
        (void)LL_SPI_ReceiveData8(SPI2);
    }
    return;
}

bool SPI2_Init (void) {
    /* Включаем тактирование SPI2 */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);

    /**
        SPI2 GPIO Configuration
            PB13    ------> SPI2_SCK
            PB14    ------> SPI2_MISO
            PB15    ------> SPI2_MOSI
            PA15    ------> nCS_MON (PushPull)
            PB6     ------> nCS_LCD (OpenDrain)
    */

    LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin =  nCS_MON.pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    if (LL_GPIO_Init(nCS_MON.port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }
    LL_GPIO_SetOutputPin(nCS_MON.port, nCS_MON.pin);

    GPIO_InitStruct.Pin = nCS_LCD.pin;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    if (LL_GPIO_Init(nCS_LCD.port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }
    LL_GPIO_SetOutputPin(nCS_LCD.port, nCS_LCD.pin);

    GPIO_InitStruct.Pin =  SPI2_SCK_GPIO_Pin | SPI2_MOSI_GPIO_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    if (LL_GPIO_Init(SPI2_SCK_GPIO_Port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    GPIO_InitStruct.Pin = SPI2_MISO_GPIO_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    if (LL_GPIO_Init(SPI2_MISO_GPIO_Port, &GPIO_InitStruct) != SUCCESS) {
        return false;
    }

    /* Сразу выставляем режим по умолчанию и включаем шину */
    current_mode = (uint8_t)SPI2_DEFAULT_MODE;
    current_direction = (uint8_t)SPI2_DEFAULT_DIRECTION;
    LL_SPI_Disable(SPI2);
    spi2_apply_config(SPI2_DEFAULT_MODE, SPI2_DEFAULT_DIRECTION);
    return LL_SPI_IsEnabled(SPI2) ? true : false;
}



static SPIController_Status spi2_capture (SPIDevices requested_device);
static void spi2_release (void);
static SPIController_Status spi2_operate (const uint8_t *data, 
                                          const uint32_t datasize, 
                                          uint8_t *ans, 
                                          const uint32_t anssize, 
                                          const uint32_t timeout);
static void spi2_complete (void);
static void spi2_signalize (void);
static bool spi2_is_busy (void);
static void spi2_set_signal (volatile bool *user_signal);
static SPIController_Status spi2_wait_done (uint32_t timeout_ms);

const SPIController_TypeDef spi2ctrl = {
    .capture = spi2_capture,
    .release = spi2_release,
    .operate = spi2_operate,
    .complete = spi2_complete,
    .signalize = spi2_signalize,
    .set_signal = spi2_set_signal,
    .is_busy = spi2_is_busy,
    .wait_done = spi2_wait_done
};



static SPIController_Status spi2_capture (SPIDevices requested_device) {
    /*
     * Проверка и установка флага - одной неделимой операцией. capture()
     * зовут и из главного цикла (экран), и из прерывания TIM3 (INA239);
     * без запрета прерываний оба могут решить, что владеют шиной.
     * PRIMASK сохраняем и восстанавливаем, чтобы вызов был безопасен и
     * изнутри обработчика.
     */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    bool taken = captured;
    if (!taken) {
        captured = true;
        holded_device = requested_device;
    }

    __set_PRIMASK(primask);

    if (taken) {
        return SPICtrlStatus_Deny;
    }

    /* Шина наша - отобрать её больше некому, настраиваем спокойно */
    xfer_done = false;
    spi2_drain_rx();
    spi2_apply_device_config(requested_device);
    return SPICtrlStatus_Allow;
}

static void spi2_release (void) {
    /* Отпускать невзятую шину нечего: обработчики DMA зовут release()
     * безусловно, а прерывание может прийти и на уже свободную шину. */
    if (!captured) {
        return;
    }

    /*
     * Короткая страховка на случай, если сдвиговый регистр ещё занят.
     * Полноценное ожидание тут делать нельзя: release() зовут в том числе
     * обработчики DMA. Для полудуплексной передачи ожидание вынесено к
     * владельцу - в wait_done(), см. SPI2_Release_Policy.
     */
    for (uint32_t guard = SPI2_BSY_GUARD; guard && LL_SPI_IsActiveFlag_BSY(SPI2); guard--) {}

    switch (holded_device) {
        case INA239: 
            LL_GPIO_SetOutputPin(nCS_MON.port, nCS_MON.pin);
            DMA1_SPI2RX_Stop();
            DMA1_SPI2RX_DenyInterrupt();
            DMA1_SPI2TX_Stop();
            break;

        case LCD: 
            LL_GPIO_SetOutputPin(nCS_LCD.port, nCS_LCD.pin);
            DMA1_SPI2TX_Stop();
            DMA1_SPI2TX_DenyInterrupt();
            break;

        default: break;
    }

    /*
     * Шину не выключаем. Возвращаем режим по умолчанию, чтобы синхронный
     * обмен с INA239 (он идёт без DMA и без capture-хозяина в лице operate)
     * всегда заставал её готовой.
     */
    spi2_drain_rx();
    spi2_apply_config(SPI2_DEFAULT_MODE, SPI2_DEFAULT_DIRECTION);

    signal = NULL;
    xfer_done = false;
    captured = false;
    return;
}

static SPIController_Status spi2_operate (const uint8_t *data, 
                                          const uint32_t datasize, 
                                          uint8_t *ans, 
                                          const uint32_t anssize, 
                                          const uint32_t timeout) {
    (void)timeout;

    if ((data == NULL) || (datasize == 0)) {
        return SPICtrlStatus_Deny;
    }

    switch (holded_device) {
        case INA239: {
            if ((ans == NULL) || (anssize == 0)) {
                return SPICtrlStatus_Deny;
            }
            /*
             * Полный дуплекс: последний ПРИНЯТЫЙ байт означает, что кадр уже
             * вытактован. Ждать нечего, шину отпускает сам обработчик DMA.
             */
            release_policy = SPI2_ReleaseInIsr;
            LL_GPIO_ResetOutputPin(nCS_MON.port, nCS_MON.pin);

            DMA1_SPI2RX_Init((uint32_t)ans, anssize);
            DMA1_SPI2RX_AllowInterrupt();
            DMA1_SPI2RX_Start();

            DMA1_SPI2TX_Init((uint32_t)data, datasize);
            DMA1_SPI2TX_Start();
            return SPICtrlStatus_Allow;
        }

        case LCD: {
            /*
             * Полудуплекс на передачу: после прерывания DMA последний байт
             * ещё в сдвиговом регистре. Обработчик только отметит завершение,
             * а доведёт и отпустит шину владелец через wait_done().
             */
            release_policy = SPI2_ReleaseByOwner;
            LL_GPIO_ResetOutputPin(nCS_LCD.port, nCS_LCD.pin);

            DMA1_SPI2TX_Init((uint32_t)data, datasize);
            DMA1_SPI2TX_AllowInterrupt();
            DMA1_SPI2TX_Start();
            return SPICtrlStatus_Allow;
        }

        default: return SPICtrlStatus_Deny;
    }
}

static void spi2_signalize (void) {
    if (signal) {
        *signal = true;
    }
    return;
}

static void spi2_complete (void) {
    xfer_done = true;
    spi2_signalize();

    if (release_policy == SPI2_ReleaseInIsr) {
        spi2_release();
    }
    return;
}

static bool spi2_is_busy (void) {
    return captured;
}

static void spi2_set_signal (volatile bool *user_signal) {
    if (user_signal) {
        signal = user_signal;
    }
    return;
}

static SPIController_Status spi2_wait_done (uint32_t timeout_ms) {
    uint8_t delay_id = 0;
    if (delay_register_task(&delay_id, timeout_ms, Millis)) {
        /* Ждём, пока DMA выложит все байты */
        while (!xfer_done) {
            if (!captured) {
                /* Шину уже отпустили за нас - обмен закончен */
                return SPICtrlStatus_Allow;
            }
            if (!delay_is_not_yet_expired(delay_id)) {
                spi2_release();
                return SPICtrlStatus_Timeout;
            }
        }

        /*
        * Теперь ждём, пока последний байт реально уйдёт из сдвигового регистра.
        * Это главный цикл, ждать здесь можно - в отличие от обработчика DMA,
        * который на это время заблокировал бы систему.
        */
        while (LL_SPI_IsActiveFlag_BSY(SPI2)) {
            if (!delay_is_not_yet_expired(delay_id)) {
                spi2_release();
                return SPICtrlStatus_Timeout;
            }
        }

        delay_unregister_task(delay_id);
        spi2_release();
        return SPICtrlStatus_Allow;
    } else {
        return SPICtrlStatus_Deny;
    }
}
