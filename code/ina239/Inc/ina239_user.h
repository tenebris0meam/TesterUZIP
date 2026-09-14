#ifndef INA239_USER_H
#define INA239_USER_H

#include "ina239.h"

INA239_Handle_t *INA239_GetInstance (void);
bool INA239_MainInit (void);
INA239_Status_t INA239_GetMode (INA239_Mode_t* res);
INA239_Status_t INA239_SetMode (INA239_Mode_t new_mode);
INA239_Status_t INA239_HandleMathOFError (void);

/** @brief Длина кадра запроса и ответа INA239, байт. */
#define INA239_FRAME_SIZE (6)

extern const uint32_t request_size;
extern const uint8_t request_vbus_and_current[];
extern volatile uint32_t answer_size;
extern volatile uint8_t answer_vbus_and_current[];
extern const uint32_t request_timeout;

#endif /* INA239_USER_H */