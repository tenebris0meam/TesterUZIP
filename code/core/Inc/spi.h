#ifndef SPI_H
#define SPI_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file spi.h
 * @brief Контроллер разделяемой шины SPI2.
 *
 * @details На шине три ведомых: INA239 (измерение, обмен по DMA из
 *          прерывания TIM3), экран NT75451 (кадр из главного цикла).
 *
 *          ПРАВИЛА, на которых всё держится:
 *
 *          1. Дверь к шине одна. Кто бы ни обращался к SPI2 - через DMA или
 *             побайтно - он сначала берёт шину через capture(), а в конце
 *             отдаёт через release(). Второго пути нет.
 *          2. capture() атомарен относительно прерываний и НЕ отбирает шину
 *             у текущего владельца. Занято - значит занято.
 *          3. capture() сам настраивает шину под запрошенное устройство:
 *             режим CPOL/CPHA и направление обмена.
 *          4. Из прерывания шину не ждут. Занято - пропускаем такт и выходим.
 *             Ожидание wait_done() - только для главного цикла.
 *          5. Между транзакциями SPI2 остаётся ВКЛЮЧЁННЫМ и настроенным на
 *             INA239: синхронный обмен идёт без DMA и застаёт шину готовой.
 *          6. В обработчике прерывания НЕ ждут опустошения сдвигового
 *             регистра. Обработчик DMA только отмечает, что байты выложены,
 *             и взводит signal. Кто именно доводит обмен до конца - зависит
 *             от устройства, см. SPI2_Release_Policy.
 */

#define SPI_MAX_BUF_SIZE (256)

typedef enum {
    SPI00,
    SPI01,
    SPI10,
    SPI11
} SPI_Modes;

typedef enum {
    SPI_FullDuplex,
    SPI_HalfDuplex_RX,
    SPI_HalfDuplex_TX
} SPI_TransferDirection;

typedef enum {
    SPICtrlStatus_Allow,
    SPICtrlStatus_Deny,
    SPICtrlStatus_Timeout,
    SPICtrlStatus_Count
} SPIController_Status;

typedef enum {
    INA239,
    LCD, 
    SPIDevices_Count
} SPIDevices;

/**
 * @brief Кто доводит обмен до конца и отпускает шину.
 * @details Прерывание DMA приходит, когда байт ЗАПИСАН в регистр данных, а не
 *          когда он сдвинут наружу. Ждать опустошения сдвигового регистра в
 *          обработчике нельзя - он на это время блокирует систему.
 */
typedef enum {
    /**
     * Полный дуплекс: признак конца даёт ПРИЁМ. Последний принятый байт
     * означает, что кадр уже вытактован, ждать нечего - шину отпускает
     * сам обработчик DMA.
     */
    SPI2_ReleaseInIsr,

    /**
     * Полудуплекс на передачу: после прерывания DMA последний байт ещё в
     * сдвиговом регистре. Обработчик только отмечает завершение, а шину
     * доводит и отпускает владелец из главного цикла через wait_done().
     */
    SPI2_ReleaseByOwner
} SPI2_Release_Policy;

typedef struct {
    /**
     * @brief Взять шину. Атомарно, без отъёма у текущего владельца.
     * @details При успехе шина уже настроена под это устройство.
     */
    SPIController_Status (*capture) (SPIDevices);

    /** @brief Отдать шину: снять CS, остановить DMA, вернуть режим по умолчанию. */
    void (*release) (void);

    /**
     * @brief Запустить обмен по DMA. Вызывать только владельцем шины.
     * @details Возвращается сразу. Буферы должны жить до конца обмена; для
     *          политики SPI2_ReleaseByOwner дождаться можно через wait_done().
     */
    SPIController_Status (*operate) (const uint8_t *data, 
                                     const uint32_t datasize, 
                                     uint8_t *ans, 
                                     const uint32_t anssize, 
                                     const uint32_t timeout);

    /**
     * @brief Отметить, что DMA закончил перекладывать байты.
     * @details Зовут обработчики DMA1_Channel4/5. Взводит signal владельца и,
     *          если политика это позволяет, отпускает шину.
     */
    void (*complete) (void);

    /** @brief Взвести флаг, переданный владельцем. */
    void (*signalize) (void);

    /** @brief Занята ли шина прямо сейчас. */
    bool (*is_busy) (void);

    /** @brief Передать пользовательский флаг */
    void (*set_signal) (volatile bool *);

    /**
     * @brief Дождаться конца обмена, довести его и отпустить шину.
     * @warning ТОЛЬКО из главного цикла: ожидание разбудит прерывание.
     * @retval SPICtrlStatus_Allow   обмен завершён, шина отпущена
     * @retval SPICtrlStatus_Timeout не дождались; шина отпущена принудительно
     */
    SPIController_Status (*wait_done) (uint32_t timeout_ms);
} SPIController_TypeDef;

extern const SPIController_TypeDef spi2ctrl;

bool SPI2_Init (void);

#endif /* SPI_H */
