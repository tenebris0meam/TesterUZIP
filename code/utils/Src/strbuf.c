#include "strbuf.h"

/** @brief Максимум разрядов у uint32_t (4294967295). */
#define STRBUF_MAX_DIGITS (10U)

void strbuf_init (StrBuf_TypeDef *sb, char *buffer, uint32_t size) {
    sb->buf = buffer;
    sb->size = size;
    sb->len = 0;
    if (size) {
        buffer[0] = '\0';
    }
    return;
}

void strbuf_clear (StrBuf_TypeDef *sb) {
    sb->len = 0;
    if (sb->size) {
        sb->buf[0] = '\0';
    }
    return;
}

void strbuf_char (StrBuf_TypeDef *sb, char c) {
    if (sb->len + 1 >= sb->size) {
        return;
    }
    sb->buf[sb->len] = c;
    sb->len++;
    sb->buf[sb->len] = '\0';
    return;
}

void strbuf_str (StrBuf_TypeDef *sb, const char *str) {
    while (*str) {
        if (sb->len + 1 >= sb->size) {
            return;
        }
        sb->buf[sb->len] = *str;
        sb->len++;
        str++;
    }
    if (sb->size) {
        sb->buf[sb->len] = '\0';
    }
    return;
}

void strbuf_strn (StrBuf_TypeDef *sb, const char *str, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (sb->len + 1 >= sb->size) {
            break;
        }
        sb->buf[sb->len] = str[i];
        sb->len++;
    }
    if (sb->size) {
        sb->buf[sb->len] = '\0';
    }
    return;
}

void strbuf_u32 (StrBuf_TypeDef *sb, uint32_t val, uint8_t min_digits) {
    char digits[STRBUF_MAX_DIGITS];
    uint8_t cnt = 0;

    do {
        digits[cnt] = (char)('0' + (val % 10U));
        cnt++;
        val /= 10U;
    } while (val && (cnt < STRBUF_MAX_DIGITS));

    for (uint8_t pad = cnt; pad < min_digits; pad++) {
        strbuf_char(sb, '0');
    }

    while (cnt) {
        cnt--;
        strbuf_char(sb, digits[cnt]);
    }
    return;
}
