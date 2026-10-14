#ifndef MULTI_POLE_TEST_H_
#define MULTI_POLE_TEST_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file multi_pole_test.h
 * @brief Неблокирующий сценарий проверки многополюсного устройства.
 *
 * Модуль управляет только операторской последовательностью:
 *   подготовка пары -> ожидание ожидаемого ALERT -> обязательная
 *   реинициализация INA239 -> countdown 3..1 -> запуск штатного теста.
 *
 * Фактическое управление PWM, настройка коммутатора, восстановление INA239,
 * отрисовка экрана и учёт результата остаются за проектным адаптером.
 * Это намеренное разделение: модуль не должен обходить существующие
 * аппаратные блокировки и штатный алгоритм измерения.
 */

typedef enum {
    MULTI_POLE_TEST_IDLE = 0,
    MULTI_POLE_TEST_WAIT_FOR_SHORT,
    MULTI_POLE_TEST_COUNTDOWN,
    MULTI_POLE_TEST_RUNNING_TEST,
    MULTI_POLE_TEST_COMPLETE,
    MULTI_POLE_TEST_ERROR
} MultiPoleTestState;

typedef enum {
    MULTI_POLE_TEST_ERROR_NONE = 0,
    MULTI_POLE_TEST_ERROR_BAD_ARGUMENT,
    MULTI_POLE_TEST_ERROR_PAIR_UNAVAILABLE,
    MULTI_POLE_TEST_ERROR_PREPARE_FAILED,
    MULTI_POLE_TEST_ERROR_UNEXPECTED_ALERT,
    MULTI_POLE_TEST_ERROR_GENERATION_STOP_FAILED,
    MULTI_POLE_TEST_ERROR_INA239_REINIT_FAILED,
    MULTI_POLE_TEST_ERROR_TEST_START_FAILED
} MultiPoleTestError;

typedef struct {
    uint8_t red_pin;   /**< Номер вывода для красного щупа, как в конфигурации. */
    uint8_t black_pin; /**< Номер вывода для чёрного щупа, как в конфигурации. */
} MultiPoleTestPair;

typedef struct {
    /** Получить пару выводов для позиции [0, pair_count). */
    bool (*get_pair)(uint8_t pair_index, MultiPoleTestPair *pair);

    /**
     * Подготовить очередную пару и включить низковольтную генерацию.
     * Должна возвращать false, если коммутация или запуск не удались.
     */
    bool (*begin_prepare)(uint8_t pair_index, const MultiPoleTestPair *pair);

    /** Остановить генерацию. Вызывается после ожидаемого ALERT и при abort. */
    bool (*stop_generation)(void);

    /**
     * Обязательная полная реинициализация INA239 после ожидаемого ALERT.
     * Callback должен восстановить конфигурацию/порог ALERT, очистить причину
     * ALERT, сбросить флаг TIM1 Break и очистить pending IRQ. При любой ошибке
     * возвращает false; countdown в этом случае не начинается.
     */
    bool (*reinitialize_ina239)(void);

    /** Запустить штатный алгоритм измерения текущей пары. */
    bool (*begin_test)(uint8_t pair_index, const MultiPoleTestPair *pair);

    /** Обновить экран. seconds_remaining имеет смысл только в COUNTDOWN. */
    void (*render)(MultiPoleTestState state,
                   uint8_t pair_index,
                   uint8_t pair_count,
                   const MultiPoleTestPair *pair,
                   uint8_t seconds_remaining,
                   MultiPoleTestError error);

    /** Запустить короткий сигнал buzzer; вызов неблокирующий. */
    void (*beep)(void);

    /** Возвращает монотонное время в миллисекундах (например HAL_GetTick). */
    uint32_t (*now_ms)(void);
} MultiPoleTestOps;

/** Инициализировать модуль; разрешено вызывать повторно из любого состояния. */
void multi_pole_test_init(void);

/**
 * Начать сценарий для заданного числа проверок.
 * ops должен оставаться действительным до завершения/сброса сценария.
 */
bool multi_pole_test_start(const MultiPoleTestOps *ops, uint8_t pair_count);

/** Вызывать каждый проход главного цикла; функция не блокирует выполнение. */
void multi_pole_test_process(void);

/**
 * Передать событие аппаратного ALERT из очереди событий.
 * Возвращает true, если ALERT принят как ожидаемое подтверждение оператора.
 * Возвращает false для всех остальных случаев: вызывающий код должен передать
 * событие штатному конечному автомату, где неожиданный ALERT станет аварией.
 */
bool multi_pole_test_handle_alert(void);

/**
 * Вызывать после того, как штатный тест текущей пары полностью завершён,
 * результат сохранён, а генерация остановлена. Перейдёт к следующей паре
 * либо завершит сценарий.
 */
void multi_pole_test_test_completed(void);

/** Безопасно прервать сценарий; генерация выключается best-effort. */
void multi_pole_test_abort(void);

MultiPoleTestState multi_pole_test_get_state(void);
MultiPoleTestError multi_pole_test_get_error(void);
uint8_t multi_pole_test_get_pair_index(void);
uint8_t multi_pole_test_get_pair_count(void);
uint8_t multi_pole_test_get_countdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MULTI_POLE_TEST_H_ */
