#ifndef STRBUF_H_
#define STRBUF_H_

#include <stdint.h>

/**
 * @brief Построитель строк с проверкой границ буфера.
 * @details Замена sprintf/strcpy/strcat для отрисовки экранов.
 */
typedef struct {
    char *buf;      /**< начало буфера                                */
    uint32_t size;  /**< полный размер буфера, включая место под '\0' */
    uint32_t len;   /**< текущая длина строки без '\0'                */
} StrBuf_TypeDef;

/**
 * @brief Привязать построитель к буферу и очистить строку.
 * @param sb     построитель
 * @param buffer буфер приёмника
 * @param size   полный размер буфера (>= 1)
 */
void strbuf_init (StrBuf_TypeDef *sb, char *buffer, uint32_t size);

/** @brief Сбросить строку в пустую, буфер оставить прежним. */
void strbuf_clear (StrBuf_TypeDef *sb);

/** @brief Дописать один символ. Лишнее молча отбрасывается. */
void strbuf_char (StrBuf_TypeDef *sb, char c);

/** @brief Дописать строку, завершённую нулём. */
void strbuf_str (StrBuf_TypeDef *sb, const char *str);

/**
 * @brief Дописать не более len символов из str.
 * @details Источник может лежать во Flash и не быть завершён нулём
 *          (так устроен MyString из конфигуратора).
 */
void strbuf_strn (StrBuf_TypeDef *sb, const char *str, uint32_t len);

/**
 * @brief Дописать беззнаковое число в десятичном виде.
 * @param min_digits минимальное число разрядов, слева дополняется нулями
 *                   (min_digits = 3 даёт поведение "%03u", 0 или 1 - "%u")
 */
void strbuf_u32 (StrBuf_TypeDef *sb, uint32_t val, uint8_t min_digits);

/** @brief Готовая строка, завершённая нулём. */
static inline char *strbuf_cstr (StrBuf_TypeDef *sb) {
    return sb->buf;
}

#endif /* STRBUF_H_ */
