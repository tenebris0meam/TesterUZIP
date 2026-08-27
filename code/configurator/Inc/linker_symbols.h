#ifndef __LINKER_SYMBOLS_H
#define __LINKER_SYMBOLS_H

/**
 * @file linker_symbols.h
 * @brief Границы секции .config, расставляемые скриптом линковщика.
 * @details Секция .config лежит в отдельном регионе Flash (CONFIG) и содержит
 *          сгенерированные структуры конфигурации. Символы нужны только для
 *          диагностики: посчитать занятый объём или CRC.
 */

extern char _sconfig[];
extern char _econfig[];

#endif // __LINKER_SYMBOLS_H
