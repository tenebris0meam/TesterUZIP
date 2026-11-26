#ifndef RESOLVER_H_
#define RESOLVER_H_

#include <stdint.h>

/** @brief Режим работы тестера. */
typedef enum {
    CheckDevice,
    CheckComponent,
    Voltmeter, 
    Ommeter, 
    VoltageGenerator, 
    Modes_Count
} Modes_TypeDef;

/** @brief Тип проверяемого компонента; от него зависят обработчики Generation и HardwareFail. */
typedef enum {
    TransientVoltageSupressor, 
    Varistor, 
    GasDischargeTube
} ResolveType;

void resolver_set_GDT (void);
void resolver_set_TVS_VAR (void);
void resolver_set_new_mode (Modes_TypeDef new_mode);

#endif /* RESOLVER_H_ */