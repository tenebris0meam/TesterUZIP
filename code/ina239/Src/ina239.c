/**
 ******************************************************************************
 * @file    ina239.c
 * @brief   Реализация драйвера INA239 для STM32 HAL (SPI, программный CS).
 *
 * Источник: TI INA239 datasheet SLYS027A, раздел 7.5 "Programming".
 *
 * Формат SPI-кадра (datasheet Figure 7-7, 7-8, Table 7-2):
 *   Байт 0: [ADDR5 ADDR4 ADDR3 ADDR2 ADDR1 ADDR0 0 R/W]
 *           R/W = 1 -> чтение, R/W = 0 -> запись
 *   Далее:  8/16/24 бита данных, MSB first.
 *
 * Электрические параметры SPI (datasheet 6.6/6.7):
 *   - Режим SPI: MODE1 (CPOL=0, CPHA=1) — данные защёлкиваются по заднему
 *     фронту SCLK ("data on MOSI sampled on falling edge of SCLK").
 *   - Частота до 10 МГц.
 *   - MSB first.
 *   Настройте hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
 *             hspi->Init.CLKPhase    = SPI_PHASE_2EDGE; на этапе MX_SPIx_Init().
 ******************************************************************************
 */

#include "ina239.h"
#include "spi.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_gpio.h"

/* =========================================================================
 *  Внутренние константы протокола
 * ========================================================================= */
/** @brief Значение бита R/W в командном байте при чтении. */
#define INA239_SPI_RW_READ_BIT   0x01U
/** @brief Значение бита R/W в командном байте при записи. */
#define INA239_SPI_RW_WRITE_BIT  0x00U

/* =========================================================================
 *  Вспомогательные функции CS
 * ========================================================================= */
/** @brief Активировать CS (низкий уровень). */
static inline void INA239_CsLow(INA239_Handle_t *hdev) {
    LL_GPIO_ResetOutputPin(hdev->cs_port, hdev->cs_pin);
}

/** @brief Деактивировать CS (высокий уровень). */
static inline void INA239_CsHigh(INA239_Handle_t *hdev) {
    LL_GPIO_SetOutputPin(hdev->cs_port, hdev->cs_pin);
}

/**
 * @brief  Формирует первый (командный) байт SPI-кадра.
 * @param  reg_addr адрес регистра, значащие 6 бит (b7..b2)
 * @param  is_read  true -> R/W=1 (чтение), false -> R/W=0 (запись)
 */
static inline uint8_t INA239_BuildCmdByte(uint8_t reg_addr, bool is_read) {
    /* [ADDR5..ADDR0][0][R/W] */
    uint8_t cmd = (uint8_t)((reg_addr & 0x3FU) << 2);
    if (is_read) {
        cmd |= INA239_SPI_RW_READ_BIT;
    } else {
        cmd &= ~INA239_SPI_RW_READ_BIT;
    }
    return cmd;
}

/**
 * @brief  Побайтовый обмен по SPI на базе LL API (полный дуплекс).
 */
static INA239_Status_t INA239_LL_SpiTransmitReceive (SPI_TypeDef *spix,
                                                     const uint8_t *tx,
                                                     uint8_t *rx,
                                                     uint16_t len, 
                                                     uint32_t timeout) {
    for (uint16_t i = 0; i < len; i++) {
        uint32_t wait_guard = timeout; /* грубый программный таймаут */

        /* Ждём готовности TX-буфера */
        while (!LL_SPI_IsActiveFlag_TXE(spix)) {
            if (--wait_guard == 0U) {
                return INA239_ERROR_SPI;
            }
        }
        LL_SPI_TransmitData8(spix, tx[i]);

        /* Ждём приёма байта */
        wait_guard = timeout;
        while (!LL_SPI_IsActiveFlag_RXNE(spix)) {
            if (--wait_guard == 0U) {
                return INA239_ERROR_SPI;
            }
        }
        rx[i] = LL_SPI_ReceiveData8(spix);
    }

    /* Дожидаемся, пока SPI реально освободится (не BUSY), прежде чем поднимать CS */
    uint32_t wait_guard = timeout;
    while (LL_SPI_IsActiveFlag_BSY(spix)) {
        if (--wait_guard == 0U) {
            return INA239_ERROR_SPI;
        }
    }

    return INA239_OK;
}

/* =========================================================================
 *  Инициализация
 * ========================================================================= */
INA239_Status_t INA239_Init (INA239_Handle_t *hdev,
                             SPI_TypeDef *hspi,
                             GPIO_TypeDef *cs_port,
                             uint32_t cs_pin,
                             uint32_t timeout_ms) {
    if (hdev == NULL || hspi == NULL || cs_port == NULL) {
        return INA239_ERROR_PARAM;
    }

    hdev->hspi    = hspi;
    hdev->cs_port = cs_port;
    hdev->cs_pin  = cs_pin;
    hdev->timeout = (timeout_ms == 0U) ? 100U : timeout_ms;

    /* CS неактивен (высокий уровень) в состоянии покоя */
    INA239_CsHigh(hdev);

    return INA239_OK;
}

INA239_Status_t INA239_CheckDeviceId (INA239_Handle_t *hdev) {
    if (hdev == NULL) {
        return INA239_ERROR_PARAM;
    }

    uint16_t manuf_id = 0U;
    uint16_t device_id = 0U;
    INA239_Status_t status;

    status = INA239_ReadManufacturerId(hdev, &manuf_id);
    if (status != INA239_OK) {
        return status;
    }

    status = INA239_ReadDeviceId(hdev, &device_id);
    if (status != INA239_OK) {
        return status;
    }

    if (manuf_id != INA239_MANUFACTURER_ID_VALUE) {
        return INA239_ERROR_BAD_ID;
    }

    /* Сравниваем только поле DIEID (биты 15-4), REV_ID может отличаться
     * между ревизиями кремния. */
    if ((device_id & 0xFFF0U) != (INA239_DEVICE_ID_VALUE & 0xFFF0U)) {
        return INA239_ERROR_BAD_ID;
    }

    return INA239_OK;
}

INA239_Status_t INA239_Reset(INA239_Handle_t *hdev) {
    /* CONFIG.RST (бит 15) = 1 генерирует программный POR, бит самоочищается
     * (datasheet 7.6.1.1). Остальные биты CONFIG при записи можно оставить 0 —
     * они всё равно будут сброшены к значениям по умолчанию. */
    return INA239_WriteRegister16(hdev, INA239_REG_CONFIG, INA239_CONFIG_RST_Msk);
}

/* =========================================================================
 *  Низкоуровневые транзакции
 * ========================================================================= */
INA239_Status_t INA239_ReadRegister16(INA239_Handle_t *hdev, uint8_t reg_addr, uint16_t *value) {
    if (hdev == NULL || value == NULL) {
        return INA239_ERROR_PARAM;
    }

    uint8_t tx[3] = {0};
    uint8_t rx[3] = {0};

    tx[0] = INA239_BuildCmdByte(reg_addr, true);
    /* tx[1], tx[2] — dummy байты для тактирования MISO */

    INA239_Status_t hal_status;

    /* Синхронный обмен идёт через ту же дверь, что и обмен по DMA:
     * иначе арбитраж шины ничего не гарантирует. Занято - выходим с
     * ошибкой, а не ждём: эта функция вызывается и из обработчика
     * TIM1_BRK приоритета 0. */
    if (spi2ctrl.capture(INA239) != SPICtrlStatus_Allow) {
        return INA239_ERROR_SPI;
    }
    INA239_CsLow(hdev);
    hal_status = INA239_LL_SpiTransmitReceive(hdev->hspi, tx, rx, 3U, hdev->timeout);
    INA239_CsHigh(hdev);
    spi2ctrl.release();

    if (hal_status != INA239_OK) {
        return INA239_ERROR_SPI;
    }

    /* rx[0] соответствует времени передачи командного байта (не значим);
     * данные регистра — rx[1] (MSB), rx[2] (LSB), см. Figure 7-7. */
    *value = (uint16_t)(((uint16_t)rx[1] << 8) | rx[2]);

    return INA239_OK;
}

INA239_Status_t INA239_WriteRegister16(INA239_Handle_t *hdev, uint8_t reg_addr, uint16_t value) {
    if (hdev == NULL) {
        return INA239_ERROR_PARAM;
    }

    uint8_t tx[3];
    tx[0] = INA239_BuildCmdByte(reg_addr, false);
    tx[1] = (uint8_t)(value >> 8);
    tx[2] = (uint8_t)(value & 0xFFU);

    uint8_t rx[3];
    INA239_Status_t hal_status;

    /* Синхронный обмен идёт через ту же дверь, что и обмен по DMA:
     * иначе арбитраж шины ничего не гарантирует. Занято - выходим с
     * ошибкой, а не ждём: эта функция вызывается и из обработчика
     * TIM1_BRK приоритета 0. */
    if (spi2ctrl.capture(INA239) != SPICtrlStatus_Allow) {
        return INA239_ERROR_SPI;
    }
    INA239_CsLow(hdev);
    hal_status = INA239_LL_SpiTransmitReceive(hdev->hspi, tx, rx, 3U, hdev->timeout);
    INA239_CsHigh(hdev);
    spi2ctrl.release();

    if (hal_status != INA239_OK) {
        return INA239_ERROR_SPI;
    }

    return INA239_OK;
}

INA239_Status_t INA239_ReadRegister24(INA239_Handle_t *hdev, uint8_t reg_addr, uint32_t *value) {
    if (hdev == NULL || value == NULL) {
        return INA239_ERROR_PARAM;
    }

    uint8_t tx[4] = {0};
    uint8_t rx[4] = {0};

    tx[0] = INA239_BuildCmdByte(reg_addr, true);

    INA239_Status_t hal_status;

    /* Синхронный обмен идёт через ту же дверь, что и обмен по DMA:
     * иначе арбитраж шины ничего не гарантирует. Занято - выходим с
     * ошибкой, а не ждём: эта функция вызывается и из обработчика
     * TIM1_BRK приоритета 0. */
    if (spi2ctrl.capture(INA239) != SPICtrlStatus_Allow) {
        return INA239_ERROR_SPI;
    }
    INA239_CsLow(hdev);
    hal_status = INA239_LL_SpiTransmitReceive(hdev->hspi, tx, rx, 4U, hdev->timeout);
    INA239_CsHigh(hdev);
    spi2ctrl.release();

    if (hal_status != INA239_OK) {
        return INA239_ERROR_SPI;
    }

    /* 24-битное значение: rx[1]=MSB, rx[2], rx[3]=LSB */
    *value = ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | (uint32_t)rx[3];

    return INA239_OK;
}

/* =========================================================================
 *  CONFIG (0x00)
 * ========================================================================= */
INA239_Status_t INA239_WriteConfig(INA239_Handle_t *hdev, uint16_t config_value) {
    return INA239_WriteRegister16(hdev, INA239_REG_CONFIG, config_value);
}

INA239_Status_t INA239_ReadConfig(INA239_Handle_t *hdev, uint16_t *config_value) {
    return INA239_ReadRegister16(hdev, INA239_REG_CONFIG, config_value);
}

INA239_Status_t INA239_SetAdcRange(INA239_Handle_t *hdev, INA239_AdcRange_t range) {
    if (hdev == NULL) {
        return INA239_ERROR_PARAM;
    }

    uint16_t config = 0U;
    INA239_Status_t status = INA239_ReadConfig(hdev, &config);
    if (status != INA239_OK) {
        return status;
    }

    config &= (uint16_t)~INA239_CONFIG_ADCRANGE_Msk;
    if (range == INA239_ADCRANGE_40_96_MV) {
        config |= INA239_CONFIG_ADCRANGE_Msk;
    }

    return INA239_WriteConfig(hdev, config);
}

/* =========================================================================
 *  ADC_CONFIG (0x01)
 * ========================================================================= */
INA239_Status_t INA239_WriteAdcConfig(INA239_Handle_t *hdev, uint16_t adc_config_value) {
    return INA239_WriteRegister16(hdev, INA239_REG_ADC_CONFIG, adc_config_value);
}

INA239_Status_t INA239_ReadAdcConfig(INA239_Handle_t *hdev, uint16_t *adc_config_value) {
    return INA239_ReadRegister16(hdev, INA239_REG_ADC_CONFIG, adc_config_value);
}

INA239_Status_t INA239_ConfigureAdc(INA239_Handle_t *hdev,
                                    INA239_Mode_t mode,
                                    INA239_ConvTime_t vbus_ct,
                                    INA239_ConvTime_t vshunt_ct,
                                    INA239_Avg_t avg) {
    if (hdev == NULL) {
        return INA239_ERROR_PARAM;
    }

    uint16_t reg = 0U;
    reg |= ((uint16_t)mode      << INA239_ADCCFG_MODE_Pos)   & INA239_ADCCFG_MODE_Msk;
    reg |= ((uint16_t)vbus_ct   << INA239_ADCCFG_VBUSCT_Pos) & INA239_ADCCFG_VBUSCT_Msk;
    reg |= ((uint16_t)vshunt_ct << INA239_ADCCFG_VSHCT_Pos)  & INA239_ADCCFG_VSHCT_Msk;
    reg |= ((uint16_t)avg       << INA239_ADCCFG_AVG_Pos)    & INA239_ADCCFG_AVG_Msk;

    return INA239_WriteAdcConfig(hdev, reg);
}

/* =========================================================================
 *  Измерительные регистры (0x04-0x08)
 * ========================================================================= */
INA239_Status_t INA239_ReadShuntVoltageRaw(INA239_Handle_t *hdev, int16_t *raw) {
    if (raw == NULL) {
        return INA239_ERROR_PARAM;
    }
    uint16_t tmp = 0U;
    INA239_Status_t status = INA239_ReadRegister16(hdev, INA239_REG_VSHUNT, &tmp);
    *raw = (int16_t)tmp;
    return status;
}

INA239_Status_t INA239_ReadShuntVoltage_uV(INA239_Handle_t *hdev,
                                            INA239_AdcRange_t range,
                                            float *shunt_uV) {
    if (shunt_uV == NULL) {
        return INA239_ERROR_PARAM;
    }

    int16_t raw = 0;
    INA239_Status_t status = INA239_ReadShuntVoltageRaw(hdev, &raw);
    if (status != INA239_OK) {
        return status;
    }

    float lsb_nv = (range == INA239_ADCRANGE_40_96_MV)
                       ? INA239_VSHUNT_LSB_RANGE1_nV
                       : INA239_VSHUNT_LSB_RANGE0_uV * 1000;

    /* lsb_nv хранит LSB в наноВольтах для сохранения точности без float
     * промежуточных ошибок; переводим итог в микровольты. */
    *shunt_uV = ((float)raw * lsb_nv) * 0.001f;

    return INA239_OK;
}

INA239_Status_t INA239_ReadBusVoltageRaw(INA239_Handle_t *hdev, int16_t *raw) {
    if (raw == NULL) {
        return INA239_ERROR_PARAM;
    }
    uint16_t tmp = 0U;
    INA239_Status_t status = INA239_ReadRegister16(hdev, INA239_REG_VBUS, &tmp);
    *raw = (int16_t)tmp;
    return status;
}

INA239_Status_t INA239_ReadBusVoltage_mV(INA239_Handle_t *hdev, float *bus_mV) {
    if (bus_mV == NULL) {
        return INA239_ERROR_PARAM;
    }

    int16_t raw = 0;
    INA239_Status_t status = INA239_ReadBusVoltageRaw(hdev, &raw);
    if (status != INA239_OK) {
        return status;
    }

    /* VBUS всегда положительный (datasheet 7.6.1.5), но храним как int16_t
     * на случай нестандартных условий; приводим к float напрямую. */
    *bus_mV = (float)raw * INA239_VBUS_LSB_uV * 0.001f;

    return INA239_OK;
}

INA239_Status_t INA239_ReadTemperatureRaw(INA239_Handle_t *hdev, int16_t *raw) {
    if (raw == NULL) {
        return INA239_ERROR_PARAM;
    }
    uint16_t tmp = 0U;
    INA239_Status_t status = INA239_ReadRegister16(hdev, INA239_REG_DIETEMP, &tmp);
    *raw = (int16_t)tmp;
    return status;
}

INA239_Status_t INA239_ReadTemperature_mC(INA239_Handle_t *hdev, int32_t *temp_mC) {
    if (temp_mC == NULL) {
        return INA239_ERROR_PARAM;
    }

    int16_t raw = 0;
    INA239_Status_t status = INA239_ReadTemperatureRaw(hdev, &raw);
    if (status != INA239_OK) {
        return status;
    }

    /* DIETEMP: значимые биты 15-4, младшие 4 бита зарезервированы (datasheet 7.6.1.6).
     * Арифметический сдвиг вправо на 4 сохраняет знак. */
    int32_t code = ((int32_t)raw) >> 4;
    *temp_mC = (int32_t)((float)code * INA239_DIETEMP_LSB_MC);

    return INA239_OK;
}

INA239_Status_t INA239_ReadCurrentRaw(INA239_Handle_t *hdev, int16_t *raw) {
    if (raw == NULL) {
        return INA239_ERROR_PARAM;
    }
    uint16_t tmp = 0U;
    INA239_Status_t status = INA239_ReadRegister16(hdev, INA239_REG_CURRENT, &tmp);
    *raw = (int16_t)tmp;
    return status;
}

INA239_Status_t INA239_ReadPowerRaw(INA239_Handle_t *hdev, uint32_t *raw) {
    return INA239_ReadRegister24(hdev, INA239_REG_POWER, raw);
}

/* =========================================================================
 *  SHUNT_CAL (0x02)
 * ========================================================================= */
INA239_Status_t INA239_ReadShuntCal(INA239_Handle_t *hdev, uint16_t *shunt_cal) {
    return INA239_ReadRegister16(hdev, INA239_REG_SHUNT_CAL, shunt_cal);
}

INA239_Status_t INA239_WriteShuntCal(INA239_Handle_t *hdev, uint16_t shunt_cal) {
    /* Бит 15 зарезервирован (datasheet 7.6.1.3), маскируем на всякий случай. */
    return INA239_WriteRegister16(hdev, INA239_REG_SHUNT_CAL, (uint16_t)(shunt_cal & 0x7FFFU));
}

/* =========================================================================
 *  DIAG_ALRT (0x0B)
 * ========================================================================= */
INA239_Status_t INA239_ReadDiagAlert(INA239_Handle_t *hdev, uint16_t *diag_alrt) {
    return INA239_ReadRegister16(hdev, INA239_REG_DIAG_ALRT, diag_alrt);
}

INA239_Status_t INA239_WriteDiagAlert(INA239_Handle_t *hdev, uint16_t diag_alrt) {
    return INA239_WriteRegister16(hdev, INA239_REG_DIAG_ALRT, diag_alrt);
}

/* =========================================================================
 *  Пороговые регистры (0x0C-0x11)
 * ========================================================================= */
INA239_Status_t INA239_WriteShuntOvervoltageLimit(INA239_Handle_t *hdev, int16_t sovl_raw) {
    return INA239_WriteRegister16(hdev, INA239_REG_SOVL, (uint16_t)sovl_raw);
}

INA239_Status_t INA239_WriteShuntUndervoltageLimit(INA239_Handle_t *hdev, int16_t suvl_raw) {
    return INA239_WriteRegister16(hdev, INA239_REG_SUVL, (uint16_t)suvl_raw);
}

INA239_Status_t INA239_WriteBusOvervoltageLimit(INA239_Handle_t *hdev, uint16_t bovl_raw) {
    /* Бит 15 зарезервирован (datasheet 7.6.1.12), значимые биты 14-0. */
    return INA239_WriteRegister16(hdev, INA239_REG_BOVL, (uint16_t)(bovl_raw & 0x7FFFU));
}

INA239_Status_t INA239_WriteBusUndervoltageLimit(INA239_Handle_t *hdev, uint16_t buvl_raw) {
    /* Бит 15 зарезервирован (datasheet 7.6.1.13), значимые биты 14-0. */
    return INA239_WriteRegister16(hdev, INA239_REG_BUVL, (uint16_t)(buvl_raw & 0x7FFFU));
}

INA239_Status_t INA239_WriteTempLimit(INA239_Handle_t *hdev, int16_t temp_limit_raw) {
    /* TOL занимает биты 15-4 (datasheet 7.6.1.14): сдвигаем код влево на 4,
     * младшие 4 бита остаются зарезервированными нулями. */
    uint16_t reg_val = (uint16_t)((int32_t)temp_limit_raw << 4);
    return INA239_WriteRegister16(hdev, INA239_REG_TEMP_LIMIT, reg_val);
}

INA239_Status_t INA239_WritePowerLimit(INA239_Handle_t *hdev, uint16_t pwr_limit_raw) {
    return INA239_WriteRegister16(hdev, INA239_REG_PWR_LIMIT, pwr_limit_raw);
}

/* =========================================================================
 *  Идентификация (0x3E, 0x3F)
 * ========================================================================= */
INA239_Status_t INA239_ReadManufacturerId(INA239_Handle_t *hdev, uint16_t *manuf_id) {
    return INA239_ReadRegister16(hdev, INA239_REG_MANUFACTURER_ID, manuf_id);
}

INA239_Status_t INA239_ReadDeviceId(INA239_Handle_t *hdev, uint16_t *device_id) {
    return INA239_ReadRegister16(hdev, INA239_REG_DEVICE_ID, device_id);
}
