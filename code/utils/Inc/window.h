#ifndef MEDIAN_FILTER_H_
#define MEDIAN_FILTER_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Окно фиксированного числа элементов в линейном буфере. */
typedef struct {
    uint32_t beg;  /**< Смещение первого элемента, байт */
    uint32_t end;  /**< Смещение конца данных, байт */
    uint32_t cnt;  /**< Число элементов в окне */
    uint32_t num_elements;  /**< Ёмкость окна, элементов */
    uint32_t element_size;  /**< Размер элемента, байт */
    void *buf;  /**< Буфер под элементы */
} Window_TypeDef;

void window_init (Window_TypeDef *hfilt, 
                  uint32_t el_size, 
                  uint32_t num_el, 
                  void *buffer);
void window_push (Window_TypeDef *hfilt, void *element);
void window_clear (Window_TypeDef *hfilt);
void window_reset (Window_TypeDef *hfilt);

#endif /* MEDIAN_FILTER_H_ */