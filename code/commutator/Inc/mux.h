#ifndef MUX_H
#define MUX_H

#include "gpio.h"

typedef enum {
    ActiveLine1,
    ActiveLine2,
    ActiveLine3,
    ActiveLine4,
    ActiveLine5,
    ActiveLine6,
    ActiveLine7,
    ActiveLine8
} MUX_ActiveLine;

typedef struct {
    const Pin_TypeDef *const Enable;
    const Pin_TypeDef *const A;
    const Pin_TypeDef *const B;
    const Pin_TypeDef *const C;
} Multiplexer_TypeDef;

extern const Multiplexer_TypeDef MUX_POS;
extern const Multiplexer_TypeDef MUX_NEG;

void mux_set_active_line (const Multiplexer_TypeDef *mux, 
                          MUX_ActiveLine new_activeline);
void mux_enable_operation (const Multiplexer_TypeDef *mux);
void mux_disable_operation (const Multiplexer_TypeDef *mux);
void mux_forcedisable_operation (const Multiplexer_TypeDef *mux);

#endif /* MUX_H */