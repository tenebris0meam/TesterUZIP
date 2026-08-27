#include <sys/stat.h>

int _close_r(void *reent, int file) {
    return -1;   // ошибка
}

int _lseek_r(void *reent, int file, int ptr, int dir) {
    return 0;    // успешно, но без перемещения
}

int _read_r(void *reent, int file, char *ptr, int len) {
    return 0;    // ничего не прочитано
}

int _write_r(void *reent, int file, const char *ptr, int len) {
    return -1;
}