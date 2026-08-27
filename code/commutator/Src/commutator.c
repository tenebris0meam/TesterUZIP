#include "commutator.h"
#include "mux.h"
#include "params.h"
#include "relay.h"
#include "utils.h"



const Commutator_TypeDef commutator = {
    .probe_ctrl = ProbeEnable,
    .eth_ctrl = RJ45Enable, 
    .positive_datalines_ctrl = &MUX_POS, 
    .negative_datalines_ctrl = &MUX_NEG, 
    .PE_positive_ctrl = RJ45Shield_Pos, 
    .PE_negative_ctrl = RJ45Shield_Neg, 
};

static Param_Connector active_interface;
static Commutator_ActiveLine active_positive_line;
static Commutator_ActiveLine active_negative_line;
static bool commutation_allowed;

void commutator_set_active_interface (Param_Connector new_active_interface) {
    active_interface = new_active_interface;
    return;
}

void commutator_set_active_line (Commutator_ActiveLine new_line, 
                                 enum Commutator_LineType line_type) {
    switch (active_interface) {
        case Probe: 
            if (new_line >= ActiveLine_3) {
                // deny setting
                return;
            }
            break;
        case RJ45: break;
        case K1: 
            if (new_line >= ActiveLine_6) {
                // deny_setting
                return;
            }
            break;
        default: 
            // deny setting
            return;
    }
    switch (line_type) {
        case LineType_Positive: active_positive_line = new_line; break;
        case LineType_Negative: active_negative_line = new_line; break;
        default: 
            // deny setting
            return;
    }
    return;
}

Commutator_ActiveLine commutator_get_active_line (enum Commutator_LineType line_type) {
    switch (line_type) {
        case LineType_Positive: return active_positive_line;
        case LineType_Negative: return active_negative_line;
        default: return ActiveLine_None;
    }
}

void commutator_allow_commutation (void) {
    if (!commutation_allowed) {
        commutation_allowed = true;
        switch (active_interface) {
            case RJ45: 
            case K1: 
                if (!relay_get_state(commutator.eth_ctrl)) {
                    relay_set_state(commutator.eth_ctrl, true);
                    if (active_positive_line == ActiveLine_9) {
                        relay_set_state(commutator.PE_positive_ctrl, true);
                    } else {
                        mux_set_active_line(commutator.positive_datalines_ctrl, 
                                            convert_activeline_commutator2mux(active_positive_line));
                        mux_enable_operation(commutator.positive_datalines_ctrl);
                    }
                    if (active_negative_line == ActiveLine_9) {
                        relay_set_state(commutator.PE_negative_ctrl, true);
                    } else {
                        mux_set_active_line(commutator.negative_datalines_ctrl, 
                                            convert_activeline_commutator2mux(active_negative_line));
                        mux_enable_operation(commutator.negative_datalines_ctrl);
                    }
                }
                return;
            case Probe: 
                if (!relay_get_state(commutator.probe_ctrl) && 
                    (active_positive_line == ActiveLine_1 || 
                     active_positive_line == ActiveLine_2) && 
                    (active_negative_line == ActiveLine_1 || 
                     active_negative_line == ActiveLine_2)) {
                    relay_set_state(commutator.probe_ctrl, true);
                }
                return;
            default: 
                // deny allowing
                return;
        }
    }
}

void commutator_deny_commutation (void) {
    if (commutation_allowed) {
        commutation_allowed = false;
        switch (active_interface) {
            case RJ45: 
            case K1: 
                if (relay_get_state(commutator.eth_ctrl)) {
                    relay_set_state(commutator.eth_ctrl, false);
                    if (active_positive_line == ActiveLine_9) {
                        relay_set_state(commutator.PE_positive_ctrl, false);
                    } else {
                        mux_disable_operation(commutator.positive_datalines_ctrl);
                        mux_set_active_line(commutator.positive_datalines_ctrl, 
                                            ActiveLine1);
                    }
                    if (active_negative_line == ActiveLine_9) {
                        relay_set_state(commutator.PE_negative_ctrl, false);
                    } else {
                        mux_disable_operation(commutator.negative_datalines_ctrl);
                        mux_set_active_line(commutator.negative_datalines_ctrl, 
                                            ActiveLine1);
                    }
                }
                return;
            case Probe: 
                relay_set_state(commutator.probe_ctrl, false);
                return;
            default: 
                // deny allowing
                return;
        }
    }
}

void commutator_forcedeny_commutation (void) {
    /*
     * Порядок: сначала снимаем силовые реле - именно они подключают генератор
     * к испытуемому прибору, - потом мультиплексоры линий данных.
     */
    relay_force_off_all();

    mux_disable_operation(commutator.positive_datalines_ctrl);
    mux_disable_operation(commutator.negative_datalines_ctrl);
    mux_set_active_line(commutator.positive_datalines_ctrl, ActiveLine1);
    mux_set_active_line(commutator.negative_datalines_ctrl, ActiveLine1);

    active_positive_line = ActiveLine_None;
    active_negative_line = ActiveLine_None;
    commutation_allowed = false;
    return;
}
