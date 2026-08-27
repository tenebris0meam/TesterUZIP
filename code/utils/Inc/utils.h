#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "ina239.h"

uint32_t get_val_from_conv_time (INA239_ConvTime_t conv_time_enum);
uint32_t get_val_from_avg (INA239_Avg_t avg_enum);

#include "mux.h"
#include "commutator.h"

MUX_ActiveLine convert_activeline_commutator2mux (Commutator_ActiveLine);
Commutator_ActiveLine convert_activeline_mux2commutator (MUX_ActiveLine);

#endif /* UTILS_H */