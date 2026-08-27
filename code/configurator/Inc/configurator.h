#ifndef CONFIGURATOR_H_
#define CONFIGURATOR_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Размер региона CONFIG во Flash (см. STM32F103XB_FLASH.ld). */
#define MAX_CONFIG_SIZE (0x2000UL)

/**
 * @brief Проверить блок конфигурации во Flash.
 * @details Разбора здесь нет: структуры конфигурации формирует на этапе сборки
 *          tools/minify_json.py, прошивка только убеждается, что блок на месте
 *          и его формат тот, который она понимает.
 * @retval true  конфигурация пригодна к использованию
 * @retval false сигнатура, версия или содержимое не годятся
 */
bool configurator_read_config (void);

/** @brief Объём, занятый секцией .config во Flash, байт. */
uint32_t configurator_get_config_size (void);

#endif /* CONFIGURATOR_H_ */
