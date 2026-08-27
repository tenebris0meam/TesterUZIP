#ifndef MEDIAN_FILTER_H_
#define MEDIAN_FILTER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t beg;
    uint32_t end;
    uint32_t cnt;
    uint32_t num_elements;
    uint32_t element_size;
    void *buf;
} Window_TypeDef;

void window_init (Window_TypeDef *hfilt, 
                  uint32_t el_size, 
                  uint32_t num_el, 
                  void *buffer);
void window_push (Window_TypeDef *hfilt, void *element);
void window_clear (Window_TypeDef *hfilt);
void window_reset (Window_TypeDef *hfilt);

#endif /* MEDIAN_FILTER_H_ */