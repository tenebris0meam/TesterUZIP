#include "main.h"
#include "ina239_user.h"
#include "ina239.h"
#include "delay.h"

#define REQ_SIZE (INA239_FRAME_SIZE)

INA239_Handle_t ina239 = { 0 };

const uint32_t request_size = REQ_SIZE;
const uint8_t request_vbus_and_current[REQ_SIZE] = { 0x15, 0x00, 0x00, 0x11, 0x00, 0x00 };
volatile uint32_t answer_size;
volatile uint8_t answer_vbus_and_current[REQ_SIZE];
const uint32_t request_timeout = 1;



INA239_Handle_t *INA239_GetInstance (void) {
    return &ina239;
}

bool INA239_MainInit (void) {
    if (INA239_Init(&ina239, 
                    SPI2, 
                    nCS_MON_GPIO_Port, 
                    nCS_MON_GPIO_Pin, 
                    1000) != INA239_OK) {
        return false;
    }
    
    if (INA239_CheckDeviceId(&ina239) != INA239_OK) {
        return false;
    }

    /* Программный сброс до заведомо известного состояния */
    if (INA239_Reset(&ina239) != INA239_OK) {
        return false;
    }
    delay_ms(1); /* TPOR ~300 us после power-up, с запасом */

    /* Вводим INA239 в shutdown mode, пока не будет инициализировано всё,
     * время преобразования 50 мкс на канал, усреднения нет */
    if (INA239_ConfigureAdc(&ina239,
                            INA239_MODE_SHUTDOWN,
                            INA239_CT_50US,
                            INA239_CT_50US,
                            INA239_CT_50US,
                            INA239_AVG_1) != INA239_OK) {
        return false;
    }

    /* Диапазон шунта: ±163.84 мВ (ADCRANGE = 0) */
    if (INA239_SetAdcRange(&ina239, INA239_ADCRANGE_163_84_MV) != INA239_OK) {
        return false;
    }

    /* SHUNT_CAL — расчёт по Rshunt/Imax НЕ делается драйвером,
     * значение должно быть подготовлено заранее (см. datasheet 8.1.2). */
    /* Считываем только падение напряжения на шунте */
    if (INA239_WriteShuntCal(&ina239, 0) != INA239_OK) {
        return false;
    }

    /* Пороги защиты (сырые коды, тоже посчитаны заранее пользователем) */
    if (INA239_WriteShuntOvervoltageLimit(&ina239, SOVL_RAW) != INA239_OK) {
        return false;
    }
    if (INA239_WriteShuntUndervoltageLimit(&ina239, -SOVL_RAW) != INA239_OK) {
        return false;
    }
    if (INA239_WriteBusOvervoltageLimit(&ina239, BOVL_RAW) != INA239_OK) {
        return false;
    }
    if (INA239_WriteBusUndervoltageLimit(&ina239, 0) != INA239_OK) {
        return false;
    }

    uint16_t diag_cfg = 0U;
    if (INA239_ReadDiagAlert(&ina239, &diag_cfg) != INA239_OK) {
        return false;
    }
    diag_cfg |= INA239_DIAG_ALATCH_Msk; 
    diag_cfg &= ~INA239_DIAG_CNVR_Msk; // Выключение CONV_RDY на пине alert
    diag_cfg &= ~INA239_DIAG_SLOWALERT_Msk; // Для функции ALERT выбирать не усреднённые значения
    diag_cfg &= ~INA239_DIAG_APOL_Msk; // Полярность выхода нормальная - active low, open drain
    if (INA239_WriteDiagAlert(&ina239, diag_cfg) != INA239_OK) {
        return false;
    }
    return true;
}

INA239_Status_t INA239_GetMode (INA239_Mode_t* res) {
    uint16_t cur_adc_cfg = 0;
    if (INA239_ReadAdcConfig(&ina239, &cur_adc_cfg) != INA239_OK) {
        return INA239_ERROR_SPI;
    }
    cur_adc_cfg &= INA239_ADCCFG_MODE_Msk;
    cur_adc_cfg >>= INA239_ADCCFG_MODE_Pos;
    *res = (INA239_Mode_t)cur_adc_cfg;

    return INA239_OK;
}

INA239_Status_t INA239_SetMode (INA239_Mode_t new_mode) {
    uint16_t cur_adc_cfg = 0;
    if (INA239_ReadAdcConfig(&ina239, &cur_adc_cfg) != INA239_OK) {
        return INA239_ERROR_SPI;
    }
    cur_adc_cfg &= ~INA239_ADCCFG_MODE_Msk;
    cur_adc_cfg |= ((uint16_t)new_mode << INA239_ADCCFG_MODE_Pos);
    if (INA239_WriteAdcConfig(&ina239, cur_adc_cfg) != INA239_OK) {
        return INA239_ERROR_SPI;
    }
    return INA239_OK;
}