#include "mux.h"
#include "gpio.h"
#include "delay.h"



const Multiplexer_TypeDef MUX_POS = {
    .Enable = &nMUX1_EN,
    .A = &MUX1_A,
    .B = &MUX1_B,
    .C = &MUX1_C
};

static bool operation_enable_pos = false;
static MUX_ActiveLine activeline_pos = ActiveLine1;

const Multiplexer_TypeDef MUX_NEG = {
    .Enable = &nMUX2_EN,
    .A = &MUX2_A,
    .B = &MUX2_B,
    .C = &MUX2_C
};

static bool operation_enable_neg = false;
static MUX_ActiveLine activeline_neg = ActiveLine1;



void mux_set_active_line (const Multiplexer_TypeDef *mux, 
                          MUX_ActiveLine new_activeline) {
    bool c = (bool)((uint8_t)new_activeline & 0b00000100);
    bool b = (bool)((uint8_t)new_activeline & 0b00000010);
    bool a = (bool)((uint8_t)new_activeline & 0b00000001);
    Pin_Set_State((Pin_TypeDef *)mux->C, c);
    Pin_Set_State((Pin_TypeDef *)mux->B, b);
    Pin_Set_State((Pin_TypeDef *)mux->A, a);
    if (mux == &MUX_POS) {
        activeline_pos = new_activeline;
    } else {
        activeline_neg = new_activeline;
    }
    return;
}

void mux_enable_operation (const Multiplexer_TypeDef *mux) {
    Pin_Set_State((Pin_TypeDef *)mux->Enable, true);
    delay_us(1);
    if (mux == &MUX_POS) {
        operation_enable_pos = true;
    } else {
        operation_enable_neg = true;
    }
    return;
}

void mux_disable_operation (const Multiplexer_TypeDef *mux) {
    Pin_Set_State((Pin_TypeDef *)mux->Enable, false);
    delay_us(1);
    if (mux == &MUX_POS) {
        operation_enable_pos = false;
    } else {
        operation_enable_neg = false;
    }
    return;
}