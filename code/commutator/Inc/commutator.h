#ifndef COMMUTATOR_H_
#define COMMUTATOR_H_

#include "params.h"
#include "relay.h"
#include "mux.h"



enum Commutator_LineType {
    LineType_Negative,
    LineType_Positive 
};



typedef enum {
    ActiveLine_None, 
    ActiveLine_1,   /** Probe_Red +     K1_1 +  ETH_Tx+ */
    ActiveLine_2,   /** Probe_Black +   K1_2 +  ETH_Tx- */
    ActiveLine_3,   /**                 K1_3 +  ETH_Rx+ */
    ActiveLine_4,   /**                 K1_4 +  ETH_BiDA */
    ActiveLine_5,   /**                 K1_5 +  ETH_BiDB */
    ActiveLine_6,   /**                         ETH_Rx- */
    ActiveLine_7,   /**                         ETH_BiDC */
    ActiveLine_8,   /**                         ETH_BiDD */
    ActiveLine_9    /**                         ETH_PE */
} Commutator_ActiveLine;

/**
 * @brief Схема коммутации: какие реле и мультиплексоры за что отвечают.
 */
typedef struct {
    const Relay_TypeDef probe_ctrl;  /**< Реле щупов */
    const Relay_TypeDef eth_ctrl;  /**< Реле разъёма (RJ45 и клеммник K1) */
    const Multiplexer_TypeDef *const positive_datalines_ctrl;  /**< Мультиплексор плюсовой линии */
    const Multiplexer_TypeDef *const negative_datalines_ctrl;  /**< Мультиплексор минусовой линии */
    const Relay_TypeDef PE_positive_ctrl;  /**< Реле экрана RJ45 на плюсовой линии */
    const Relay_TypeDef PE_negative_ctrl;  /**< Реле экрана RJ45 на минусовой линии */
} Commutator_TypeDef;



extern const Commutator_TypeDef commutator;



void commutator_set_active_interface (Param_Connector new_active_interface);
void commutator_set_active_line (Commutator_ActiveLine new_line, 
                                 enum Commutator_LineType line_type);
/**
 * @brief Какая линия принята коммутатором на указанный полюс.
 * @details commutator_set_active_line() отказывает молча: у щупов доступны
 *          только линии 1 и 2, у K1 - с 1 по 5. Процессор обязан
 *          убедиться, что коммутация в самом деле собрана, до того как подаст
 *          высокое напряжение, - для этого и нужен этот доступ.
 */
Commutator_ActiveLine commutator_get_active_line (enum Commutator_LineType line_type);

void commutator_allow_commutation (void);
void commutator_deny_commutation (void);

/**
 * @brief Аварийно разомкнуть коммутацию. Без задержек, безопасно из прерывания.
 */
void commutator_forcedeny_commutation (void);

#endif /* COMMUTATOR_H_ */