#ifndef RESOLVER_H_
#define RESOLVER_H_

#include <stdint.h>

typedef enum {
    CheckDevice,
    CheckComponent,
    Voltmeter, 
    Ommeter, 
    VoltageGenerator, 
    Modes_Count
} Modes_TypeDef;

typedef enum {
    TransientVoltageSupressor, 
    Varistor, 
    GasDischargeTube
} ResolveType;

void resolver_set_GDT (void);
void resolver_set_TVS_VAR (void);
void resolver_set_new_mode (Modes_TypeDef new_mode);

#endif /* RESOLVER_H_ */