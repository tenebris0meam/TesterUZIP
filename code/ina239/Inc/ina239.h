/**
 ******************************************************************************
 * @file    ina239.h
 * @brief   Драйвер для INA239 (85-V, 16-Bit, High-Precision Power Monitor,
 *          SPI Interface) под STM32 HAL.
 *
 * Источник: Texas Instruments INA239 datasheet SLYS027A (Rev. A, May 2022)
 * https://static.chipdip.ru/lib/187/DOC055187018.pdf
 *
 * Драйвер предоставляет доступ к "сырым" регистрам устройства без
 * автоматического расчёта SHUNT_CAL / CURRENT_LSB. Преобразование сырых
 * кодов в физические величины (шунт, шина, температура) выполняется по
 * фиксированным LSB-константам из datasheet (Table 8-1).
 * Пересчёт CURRENT/POWER в физические единицы (Section 8.1.2) остаётся
 * на стороне пользователя, так как зависит от выбранного Rshunt/Imax.
 ******************************************************************************
 */

#ifndef INA239_H
#define INA239_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32f103xb.h"

// #define SHUNT_MAX_CURRENT_nA (1500000) // nA
// #define SHUNT_CURRENT_LSB (46) // nA/LSB
// #define SHUNT_CONDUCTANCE_mS (45455) // mS (1 / kOhm)
// формула 819,2e6 * SHUNT_CURRENT_LSB * SHUNT_RESISTANCE_Ohm
#if defined DEBUG
#define SHUNT_CAL_RAW   0x0339L
#define SOVL_RAW        0x7530L // 150 mV limit, 160mV/5uV = 32000
#else 
#define SHUNT_CAL_RAW   0x0F5CUL // CURRENT_LSB = 32 nA/LSB, SHUNT_CAL = 3932
#define SOVL_RAW        0x7D00L // 150 mV limit, 160mV/5uV = 32000
#endif
#define BOVL_RAW        0x6720L // 82.5V limit, 82.5V/3.125mV = 26400

/* =========================================================================
 *  Адреса регистров (datasheet Table 7-3)
 * ========================================================================= */
#define INA239_REG_CONFIG          0x00U  /* Configuration,                16 бит, R/W */
#define INA239_REG_ADC_CONFIG      0x01U  /* ADC Configuration,             16 бит, R/W */
#define INA239_REG_SHUNT_CAL       0x02U  /* Shunt Calibration,             16 бит, R/W */
#define INA239_REG_VSHUNT          0x04U  /* Shunt Voltage Measurement,     16 бит, R  */
#define INA239_REG_VBUS            0x05U  /* Bus Voltage Measurement,       16 бит, R  */
#define INA239_REG_DIETEMP         0x06U  /* Temperature Measurement,       16 бит, R  */
#define INA239_REG_CURRENT         0x07U  /* Current Result,                16 бит, R  */
#define INA239_REG_POWER           0x08U  /* Power Result,                  24 бит, R  */
#define INA239_REG_DIAG_ALRT       0x0BU  /* Diagnostic Flags and Alert,    16 бит, R/W */
#define INA239_REG_SOVL            0x0CU  /* Shunt Overvoltage Threshold,   16 бит, R/W */
#define INA239_REG_SUVL            0x0DU  /* Shunt Undervoltage Threshold,  16 бит, R/W */
#define INA239_REG_BOVL            0x0EU  /* Bus Overvoltage Threshold,     16 бит, R/W */
#define INA239_REG_BUVL            0x0FU  /* Bus Undervoltage Threshold,    16 бит, R/W */
#define INA239_REG_TEMP_LIMIT      0x10U  /* Temperature Over-Limit,        16 бит, R/W */
#define INA239_REG_PWR_LIMIT       0x11U  /* Power Over-Limit Threshold,    16 бит, R/W */
#define INA239_REG_MANUFACTURER_ID 0x3EU  /* Manufacturer ID ("TI"),        16 бит, R  */
#define INA239_REG_DEVICE_ID       0x3FU  /* Device ID / Revision,          16 бит, R  */

/* Ожидаемые значения ID (datasheet 7.6.1.16 / 7.6.1.17) */
#define INA239_MANUFACTURER_ID_VALUE 0x5449U /* ASCII "TI" */
#define INA239_DEVICE_ID_VALUE       0x2391U /* DIEID=0x239, REV_ID=0x1 */

/* =========================================================================
 *  LSB / масштабные коэффициенты (datasheet Table 8-1, Section 7.6.1.x)
 * ========================================================================= */
#define INA239_VSHUNT_LSB_RANGE0_uV  5U        /* 5 uV/LSB   при ADCRANGE = 0 (±163.84 mV) */
#define INA239_VSHUNT_LSB_RANGE1_nV  1250U     /* 1.25 uV/LSB при ADCRANGE = 1 (±40.96 mV) */
#define INA239_VBUS_LSB_uV           3125U     /* 3.125 mV/LSB */
#define INA239_DIETEMP_LSB_MC        125U      /* 125 m°C/LSB */
#if defined DEBUG
#define INA239_CURRENT_LSB_nA        46U       /* 46 nA/LSB */
#else 
#define INA239_CURRENT_LSB_nA        32U       /* 32 nA/LSB */
#endif

/* =========================================================================
 *  CONFIG register (0x00) — битовые поля (datasheet 7.6.1.1)
 * ========================================================================= */
#define INA239_CONFIG_RST_Pos        15U
#define INA239_CONFIG_RST_Msk        (1U << INA239_CONFIG_RST_Pos)
#define INA239_CONFIG_CONVDLY_Pos    6U
#define INA239_CONFIG_CONVDLY_Msk    (0xFFU << INA239_CONFIG_CONVDLY_Pos) /* шаг 2 мс, 0..510 мс */
#define INA239_CONFIG_ADCRANGE_Pos   4U
#define INA239_CONFIG_ADCRANGE_Msk   (1U << INA239_CONFIG_ADCRANGE_Pos)

typedef enum {
    INA239_ADCRANGE_163_84_MV = 0, /* ±163.84 mV, 5 uV/LSB    */
    INA239_ADCRANGE_40_96_MV  = 1  /* ±40.96 mV,  1.25 uV/LSB */
} INA239_AdcRange_t;

/* =========================================================================
 *  ADC_CONFIG register (0x01) — битовые поля (datasheet 7.6.1.2)
 * ========================================================================= */
#define INA239_ADCCFG_MODE_Pos       12U
#define INA239_ADCCFG_MODE_Msk       (0xFU << INA239_ADCCFG_MODE_Pos)
#define INA239_ADCCFG_VBUSCT_Pos     9U
#define INA239_ADCCFG_VBUSCT_Msk     (0x7U << INA239_ADCCFG_VBUSCT_Pos)
#define INA239_ADCCFG_VSHCT_Pos      6U
#define INA239_ADCCFG_VSHCT_Msk      (0x7U << INA239_ADCCFG_VSHCT_Pos)
#define INA239_ADCCFG_VTCT_Pos       3U
#define INA239_ADCCFG_VTCT_Msk       (0x7U << INA239_ADCCFG_VTCT_Pos)
#define INA239_ADCCFG_AVG_Pos        0U
#define INA239_ADCCFG_AVG_Msk        (0x7U << INA239_ADCCFG_AVG_Pos)

/* MODE (bits 15-12) */
typedef enum {
    INA239_MODE_SHUTDOWN                    = 0x0,
    INA239_MODE_TRIG_BUS                    = 0x1,
    INA239_MODE_TRIG_SHUNT                  = 0x2,
    INA239_MODE_TRIG_SHUNT_BUS              = 0x3,
    INA239_MODE_TRIG_TEMP                   = 0x4,
    INA239_MODE_TRIG_TEMP_BUS               = 0x5,
    INA239_MODE_TRIG_TEMP_SHUNT             = 0x6,
    INA239_MODE_TRIG_BUS_SHUNT_TEMP         = 0x7,
    INA239_MODE_SHUTDOWN2                   = 0x8, /* дубль shutdown */
    INA239_MODE_CONT_BUS                    = 0x9,
    INA239_MODE_CONT_SHUNT                  = 0xA,
    INA239_MODE_CONT_SHUNT_BUS              = 0xB,
    INA239_MODE_CONT_TEMP                   = 0xC,
    INA239_MODE_CONT_BUS_TEMP               = 0xD,
    INA239_MODE_CONT_TEMP_SHUNT             = 0xE,
    INA239_MODE_CONT_BUS_SHUNT_TEMP         = 0xF  /* значение по умолчанию после reset */
} INA239_Mode_t;

/* Время преобразования (общий enum для VBUSCT/VSHCT/VTCT, datasheet 7.6.1.2) */
typedef enum {
    INA239_CT_50US   = 0x0,
    INA239_CT_84US   = 0x1,
    INA239_CT_150US  = 0x2,
    INA239_CT_280US  = 0x3,
    INA239_CT_540US  = 0x4,
    INA239_CT_1052US = 0x5, /* значение по умолчанию после reset */
    INA239_CT_2074US = 0x6,
    INA239_CT_4120US = 0x7
} INA239_ConvTime_t;

/* Усреднение (AVG, biты 2-0) */
typedef enum {
    INA239_AVG_1    = 0x0, /* значение по умолчанию после reset */
    INA239_AVG_4    = 0x1,
    INA239_AVG_16   = 0x2,
    INA239_AVG_64   = 0x3,
    INA239_AVG_128  = 0x4,
    INA239_AVG_256  = 0x5,
    INA239_AVG_512  = 0x6,
    INA239_AVG_1024 = 0x7
} INA239_Avg_t;

/* =========================================================================
 *  DIAG_ALRT register (0x0B) — битовые поля (datasheet 7.6.1.9)
 * ========================================================================= */
#define INA239_DIAG_ALATCH_Pos     15U
#define INA239_DIAG_ALATCH_Msk     (1U << INA239_DIAG_ALATCH_Pos)
#define INA239_DIAG_CNVR_Pos       14U
#define INA239_DIAG_CNVR_Msk       (1U << INA239_DIAG_CNVR_Pos)
#define INA239_DIAG_SLOWALERT_Pos  13U
#define INA239_DIAG_SLOWALERT_Msk  (1U << INA239_DIAG_SLOWALERT_Pos)
#define INA239_DIAG_APOL_Pos       12U
#define INA239_DIAG_APOL_Msk       (1U << INA239_DIAG_APOL_Pos)
#define INA239_DIAG_MATHOF_Pos     9U
#define INA239_DIAG_MATHOF_Msk     (1U << INA239_DIAG_MATHOF_Pos)
#define INA239_DIAG_TMPOL_Pos      7U
#define INA239_DIAG_TMPOL_Msk      (1U << INA239_DIAG_TMPOL_Pos)
#define INA239_DIAG_SHNTOL_Pos     6U
#define INA239_DIAG_SHNTOL_Msk     (1U << INA239_DIAG_SHNTOL_Pos)
#define INA239_DIAG_SHNTUL_Pos     5U
#define INA239_DIAG_SHNTUL_Msk     (1U << INA239_DIAG_SHNTUL_Pos)
#define INA239_DIAG_BUSOL_Pos      4U
#define INA239_DIAG_BUSOL_Msk      (1U << INA239_DIAG_BUSOL_Pos)
#define INA239_DIAG_BUSUL_Pos      3U
#define INA239_DIAG_BUSUL_Msk      (1U << INA239_DIAG_BUSUL_Pos)
#define INA239_DIAG_POL_Pos        2U
#define INA239_DIAG_POL_Msk        (1U << INA239_DIAG_POL_Pos)
#define INA239_DIAG_CNVRF_Pos      1U
#define INA239_DIAG_CNVRF_Msk      (1U << INA239_DIAG_CNVRF_Pos)
#define INA239_DIAG_MEMSTAT_Pos    0U
#define INA239_DIAG_MEMSTAT_Msk    (1U << INA239_DIAG_MEMSTAT_Pos)

/* =========================================================================
 *  Коды результата операций драйвера
 * ========================================================================= */
typedef enum {
    INA239_OK                = 0,
    INA239_ERROR_SPI         = 1, /* ошибка транзакции SPI (HAL_ERROR/HAL_TIMEOUT) */
    INA239_ERROR_BAD_ID      = 2, /* MANUFACTURER_ID / DEVICE_ID не совпал с ожидаемым */
    INA239_ERROR_PARAM       = 3  /* неверный аргумент (напр. NULL-указатель) */
} INA239_Status_t;

/* =========================================================================
 *  Хэндл устройства
 * ========================================================================= */
typedef struct {
    SPI_TypeDef        *hspi;      /* указатель на SPI-интерфейс HAL              */
    GPIO_TypeDef       *cs_port;  /* порт GPIO для линии CS (программный CS)     */
    uint32_t           cs_pin;   /* пин GPIO для линии CS                       */
    uint32_t           timeout; /* таймаут SPI-транзакций, мс (например, 100)  */
} INA239_Handle_t;

/* =========================================================================
 *  Инициализация / базовые операции
 * ========================================================================= */

/**
 * @brief  Инициализация хэндла драйвера INA239.
 * @param  hdev     указатель на структуру хэндла (заполняется данной функцией)
 * @param  hspi     указатель на инициализированный SPI HAL handle (Mode 1, CPOL=0/CPHA=1,
 *                   см. примечание в ina239.c)
 * @param  cs_port  GPIO порт линии CS
 * @param  cs_pin   GPIO пин линии CS
 * @param  timeout_ms таймаут SPI операций в мс
 * @retval INA239_OK при успехе, иначе код ошибки
 */
INA239_Status_t INA239_Init(INA239_Handle_t *hdev,
                             SPI_TypeDef *hspi,
                             GPIO_TypeDef *cs_port,
                             uint32_t cs_pin,
                             uint32_t timeout_ms);

/**
 * @brief  Проверка присутствия устройства на шине по MANUFACTURER_ID/DEVICE_ID.
 * @retval INA239_OK если ID совпали, INA239_ERROR_BAD_ID если нет,
 *         INA239_ERROR_SPI при ошибке шины.
 */
INA239_Status_t INA239_CheckDeviceId(INA239_Handle_t *hdev);

/**
 * @brief  Программный полный сброс устройства (эквивалент POR).
 *         Устанавливает CONFIG.RST = 1. Регистры возвращаются к значениям
 *         по умолчанию (datasheet 7.4.2). Бит самоочищается.
 */
INA239_Status_t INA239_Reset(INA239_Handle_t *hdev);

/* =========================================================================
 *  Низкоуровневый доступ к регистрам (сырые данные)
 * ========================================================================= */

/**
 * @brief  Чтение 16-битного регистра.
 * @param  reg_addr  адрес регистра (6 бит, INA239_REG_*)
 * @param  value     указатель, куда будет записано прочитанное значение
 */
INA239_Status_t INA239_ReadRegister16(INA239_Handle_t *hdev, uint8_t reg_addr, uint16_t *value);

/**
 * @brief  Запись 16-битного регистра.
 * @param  reg_addr  адрес регистра (6 бит, INA239_REG_*)
 * @param  value     значение для записи
 */
INA239_Status_t INA239_WriteRegister16(INA239_Handle_t *hdev, uint8_t reg_addr, uint16_t value);

/**
 * @brief  Чтение 24-битного регистра (используется только для POWER, 0x08).
 * @param  value  указатель на 32-битную переменную; итоговое значение занимает
 *                младшие 24 бита (старшие 8 бит равны 0).
 */
INA239_Status_t INA239_ReadRegister24(INA239_Handle_t *hdev, uint8_t reg_addr, uint32_t *value);

/* =========================================================================
 *  Регистр-специфичные обёртки (по-прежнему "сырые" коды, без пересчёта
 *  в физические единицы, кроме простого умножения на фиксированный LSB)
 * ========================================================================= */

/** @brief Запись CONFIG (0x00) целиком. */
INA239_Status_t INA239_WriteConfig(INA239_Handle_t *hdev, uint16_t config_value);
/** @brief Чтение CONFIG (0x00) целиком. */
INA239_Status_t INA239_ReadConfig(INA239_Handle_t *hdev, uint16_t *config_value);

/** @brief Запись ADC_CONFIG (0x01) целиком. */
INA239_Status_t INA239_WriteAdcConfig(INA239_Handle_t *hdev, uint16_t adc_config_value);
/** @brief Чтение ADC_CONFIG (0x01) целиком. */
INA239_Status_t INA239_ReadAdcConfig(INA239_Handle_t *hdev, uint16_t *adc_config_value);

/**
 * @brief  Сборка значения ADC_CONFIG из отдельных полей и запись в регистр.
 *         Пересчёт SHUNT_CAL не выполняется — пользователь при необходимости
 *         пишет SHUNT_CAL отдельно через INA239_WriteRegister16().
 */
INA239_Status_t INA239_ConfigureAdc(INA239_Handle_t *hdev,
                                     INA239_Mode_t mode,
                                     INA239_ConvTime_t vbus_ct,
                                     INA239_ConvTime_t vshunt_ct,
                                     INA239_ConvTime_t vtemp_ct,
                                     INA239_Avg_t avg);

/** @brief Установка диапазона шунта (CONFIG.ADCRANGE), остальные поля CONFIG сохраняются. */
INA239_Status_t INA239_SetAdcRange(INA239_Handle_t *hdev, INA239_AdcRange_t range);

/** @brief Чтение сырого кода VSHUNT (0x04), two's complement. */
INA239_Status_t INA239_ReadShuntVoltageRaw(INA239_Handle_t *hdev, int16_t *raw);

/**
 * @brief  Чтение VSHUNT и пересчёт в микровольты по фиксированному LSB
 *         (5 uV или 1.25 uV в зависимости от текущего ADCRANGE, который
 *         нужно передать явно — драйвер его не кэширует).
 */
INA239_Status_t INA239_ReadShuntVoltage_uV(INA239_Handle_t *hdev,
                                            INA239_AdcRange_t range,
                                            float *shunt_uV);

/** @brief Чтение сырого кода VBUS (0x05), two's complement, но всегда >= 0. */
INA239_Status_t INA239_ReadBusVoltageRaw(INA239_Handle_t *hdev, int16_t *raw);

/** @brief Чтение VBUS и пересчёт в милливольты (LSB = 3.125 mV). */
INA239_Status_t INA239_ReadBusVoltage_mV(INA239_Handle_t *hdev, float *bus_mV);

/** @brief Чтение сырого кода DIETEMP (0x06). Значимые биты 15-4, младшие 4 бита зарезервированы. */
INA239_Status_t INA239_ReadTemperatureRaw(INA239_Handle_t *hdev, int16_t *raw);

/**
 * @brief  Чтение температуры и пересчёт в миллиградусы Цельсия (LSB = 125 m°C).
 *         Регистр хранит значение в битах 15-4, поэтому сырой код сдвигается
 *         вправо на 4 перед умножением на LSB.
 */
INA239_Status_t INA239_ReadTemperature_mC(INA239_Handle_t *hdev, int32_t *temp_mC);

/** @brief Чтение сырого кода CURRENT (0x07), two's complement. Пересчёт в А — на стороне пользователя (нужен CURRENT_LSB). */
INA239_Status_t INA239_ReadCurrentRaw(INA239_Handle_t *hdev, int16_t *raw);

/** @brief Чтение сырого кода POWER (0x08), 24 бита, unsigned. Пересчёт в Вт — на стороне пользователя. */
INA239_Status_t INA239_ReadPowerRaw(INA239_Handle_t *hdev, uint32_t *raw);

/** @brief Чтение/запись SHUNT_CAL (0x02) в сыром виде (без вычисления по Rshunt/Imax). */
INA239_Status_t INA239_ReadShuntCal(INA239_Handle_t *hdev, uint16_t *shunt_cal);
INA239_Status_t INA239_WriteShuntCal(INA239_Handle_t *hdev, uint16_t shunt_cal);

/** @brief Чтение регистра диагностики/алертов DIAG_ALRT (0x0B). Чтение сбрасывает защёлкнутые флаги (если ALATCH=1). */
INA239_Status_t INA239_ReadDiagAlert(INA239_Handle_t *hdev, uint16_t *diag_alrt);
/** @brief Запись конфигурационных битов DIAG_ALRT (ALATCH, CNVR, SLOWALERT, APOL). */
INA239_Status_t INA239_WriteDiagAlert(INA239_Handle_t *hdev, uint16_t diag_alrt);

/** @brief Пороговые регистры — прямой read/write "сырых" кодов (datasheet 7.6.1.10-7.6.1.15). */
INA239_Status_t INA239_WriteShuntOvervoltageLimit(INA239_Handle_t *hdev, int16_t sovl_raw);
INA239_Status_t INA239_WriteShuntUndervoltageLimit(INA239_Handle_t *hdev, int16_t suvl_raw);
INA239_Status_t INA239_WriteBusOvervoltageLimit(INA239_Handle_t *hdev, uint16_t bovl_raw);
INA239_Status_t INA239_WriteBusUndervoltageLimit(INA239_Handle_t *hdev, uint16_t buvl_raw);
INA239_Status_t INA239_WriteTempLimit(INA239_Handle_t *hdev, int16_t temp_limit_raw);
INA239_Status_t INA239_WritePowerLimit(INA239_Handle_t *hdev, uint16_t pwr_limit_raw);

/** @brief Чтение MANUFACTURER_ID (0x3E) — ожидается 0x5449 ("TI"). */
INA239_Status_t INA239_ReadManufacturerId(INA239_Handle_t *hdev, uint16_t *manuf_id);
/** @brief Чтение DEVICE_ID (0x3F) — ожидается 0x2391. */
INA239_Status_t INA239_ReadDeviceId(INA239_Handle_t *hdev, uint16_t *device_id);

#ifdef __cplusplus
}
#endif

#endif /* INA239_H */
