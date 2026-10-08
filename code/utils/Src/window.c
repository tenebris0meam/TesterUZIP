#include "window.h"

#include <string.h>

void window_init (Window_TypeDef *hfilt, 
                  uint32_t el_size, 
                  uint32_t num_el, 
                  void *buffer) {
    hfilt->beg = 0;
    hfilt->end = 0;
    hfilt->cnt = 0;
    hfilt->element_size = el_size;
    hfilt->num_elements = num_el;
    hfilt->buf = buffer;
    return;
}

void window_push (Window_TypeDef *hfilt, void *element) {
    if (hfilt->cnt < hfilt->num_elements) {
        memcpy(hfilt->buf + hfilt->end, element, hfilt->element_size);
        hfilt->end += hfilt->element_size;
        hfilt->cnt++;
    } else {
        memmove(hfilt->buf, hfilt->buf + hfilt->element_size, (hfilt->num_elements - 1) * hfilt->element_size);
        memcpy(hfilt->buf + (hfilt->num_elements - 1) * hfilt->element_size, element, hfilt->element_size);
    }
    return;
}

void window_pop (Window_TypeDef *hfilt, void *element) {
    if (hfilt->cnt > 0) {
        memcpy(element, hfilt->buf + hfilt->beg, hfilt->element_size);
        hfilt->beg += hfilt->element_size;
        hfilt->cnt--;
    }
    return;
}

void window_clear (Window_TypeDef *hfilt) {
    memset(hfilt->buf, 0, hfilt->num_elements * hfilt->element_size);
    hfilt->beg = 0;
    hfilt->cnt = 0;
    hfilt->end = 0;
    return;
}

void window_reset (Window_TypeDef *hfilt) {
    window_clear(hfilt);
    hfilt->element_size = 0;
    hfilt->num_elements = 0;
    hfilt->buf = NULL;
    return;
}