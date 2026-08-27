#include "utils.h"
#include "ina239.h"
#include <stdint.h>

uint32_t get_val_from_conv_time (INA239_ConvTime_t conv_time_enum) {
    switch (conv_time_enum) {
        case INA239_CT_50US: return 50;
        case INA239_CT_84US: return 84;
        case INA239_CT_150US: return 150;
        case INA239_CT_280US: return 280;
        case INA239_CT_540US: return 540;
        case INA239_CT_1052US: return 1052;
        case INA239_CT_2074US: return 2074;
        case INA239_CT_4120US: return 4120;
    }
}

uint32_t get_val_from_avg (INA239_Avg_t avg_enum) {
    switch (avg_enum) {
        case INA239_AVG_1: return 1;
        case INA239_AVG_4: return 4;
        case INA239_AVG_16: return 16;
        case INA239_AVG_64: return 64;
        case INA239_AVG_128: return 128;
        case INA239_AVG_256: return 256;
        case INA239_AVG_512: return 512;
        case INA239_AVG_1024: return 1024;
    }
}

#include "mux.h"
#include "commutator.h"

MUX_ActiveLine convert_activeline_commutator2mux (Commutator_ActiveLine act) {
    switch (act) {
        case ActiveLine_1: return ActiveLine1;
        case ActiveLine_2: return ActiveLine2;
        case ActiveLine_3: return ActiveLine3;
        case ActiveLine_4: return ActiveLine4;
        case ActiveLine_5: return ActiveLine5;
        case ActiveLine_6: return ActiveLine6;
        case ActiveLine_7: return ActiveLine7;
        case ActiveLine_8: return ActiveLine8;
    }
}

Commutator_ActiveLine convert_activeline_mux2commutator (MUX_ActiveLine act) {
    switch (act) {
        case ActiveLine1: return ActiveLine_1;
        case ActiveLine2: return ActiveLine_2;
        case ActiveLine3: return ActiveLine_3;
        case ActiveLine4: return ActiveLine_4;
        case ActiveLine5: return ActiveLine_5;
        case ActiveLine6: return ActiveLine_6;
        case ActiveLine7: return ActiveLine_7;
        case ActiveLine8: return ActiveLine_8;
    }
}