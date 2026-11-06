#ifndef CALC_H
#define CALC_H

#include "params.h"
#include <stdbool.h>
#include <stdint.h>

#define CALC_CURRENT_NOMINAL_nA (1000000L)
#define CALC_CURRENT_LOW_nA 900000L
#define CALC_CURRENT_HIGH_nA 2000000L
#define CALC_TARGET_TOLERANCE_PERCENT 5UL
#define CALC_DUTY_MAX_PPM (970000L)

extern volatile bool have_new_data;


typedef struct {
    int32_t voltage_uV;
    int32_t current_nA;
} CalcMetrics_TypeDef;

typedef enum {
    CalcTestMode_SweepTVS,
    CalcTestMode_SweepGDT
} CalcTestMode;

void calc_set_test_mode (CalcTestMode mode);
void calc_set_test_params (const Params_TestParam_TypeDef *tp);

bool calc_start_sweep (void);   /* регистрирует delay, ставит шаг 0 */
void calc_reset_sweep (void);   /* снимает delay, обнуляет шаг/CC */

bool calc_integrator_Init (void);
void calc_integrator_set_target_voltage (uint32_t target_voltage_after_div_uV);
void calc_integrator_set_start_time (void);
uint32_t calc_integrator_get_target_voltage (void);
int32_t calc_integrator_get_integral_value (void);
void calc_integrator_reset (void);

void calc_routine (void);
void calc_measurement_update (void);

void calc_get_test_data (int32_t *voltage, int32_t *current);
const CalcMetrics_TypeDef *calc_get_current_metrics (void);
void calc_get_last_raw_data (int32_t *voltage_raw, int32_t *current_raw);
void calc_update_metrics (void);

#endif /* CALC_H */
