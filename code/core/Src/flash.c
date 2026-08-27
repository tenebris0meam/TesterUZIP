#include "flash.h"
#include "delay.h"
#include "stm32f103xb.h"
#include "stm32f1xx.h"
#include <stdint.h>



FlashStatus flash_unlock (void) {
    if (!READ_BIT(FLASH->CR, FLASH_CR_LOCK)) {
        return Flash_OK;
    }
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    if (READ_BIT(FLASH->CR, FLASH_CR_LOCK)) {
        return Flash_OK;
    } else {
        return Flash_ErrorLocked;
    }
}

FlashStatus flash_lock (void) {
    if (READ_BIT(FLASH->CR, FLASH_CR_LOCK)) {
        return Flash_OK;
    }
    SET_BIT(FLASH->CR, FLASH_CR_LOCK);
    if (!READ_BIT(FLASH->CR, FLASH_CR_LOCK)) {
        return Flash_OK;
    } else {
        return Flash_ErrorLocked;
    }
}

FlashStatus flash_wait_operation_ends (uint32_t timeout_us) {
    uint8_t delay_id = 0;
    if (delay_register_task(&delay_id, timeout_us, Micros)) {
        while (delay_is_not_yet_expired(delay_id)) {
            if (!READ_BIT(FLASH->SR, FLASH_SR_BSY)) {
                delay_unregister_task(delay_id);
                return Flash_OK;
            }
        }
        return Flash_Timeout;
    }
    return Flash_Timeout;
}

FlashStatus flash_erase_page (uint8_t page_id, uint32_t timeout_us) {
    if (flash_wait_operation_ends(timeout_us) == Flash_OK) {
        SET_BIT(FLASH->CR, FLASH_CR_PER);
        WRITE_REG(FLASH->AR, (uint32_t)flash_get_address(page_id));
        SET_BIT(FLASH->CR, FLASH_CR_STRT);
        return flash_wait_operation_ends(timeout_us);
    } else {
        return Flash_Timeout;
    }
}

FlashStatus flash_write16 (void *address, uint16_t data, uint32_t timeout_us) {
    if (flash_wait_operation_ends(timeout_us) == Flash_OK) {
        SET_BIT(FLASH->CR, FLASH_CR_PG);
        *(__IO uint16_t *)address = data;
        return flash_wait_operation_ends(timeout_us);
    } else {
        return Flash_Timeout;
    }
}

FlashStatus flash_write_buffer (void *address, void *data, uint32_t length, uint32_t timeout_us) {
    if (flash_wait_operation_ends(timeout_us) == Flash_OK) {
        for (uint32_t idx = 0; idx < length; idx += 2) {
            if (flash_write16(address + idx, *(uint16_t *)(data + idx), timeout_us) != Flash_OK) {
                return Flash_Timeout;
            }
        }
        return Flash_OK;
    } else {
        return Flash_Timeout;
    }
}

void flash_read_buffer (void *address, void *data, uint32_t length) {
    for (uint32_t idx = 0; idx < length; idx += 2) {
        *(uint16_t *)data = flash_read16(address + idx);
    }
    return;
}

bool flash_is_page_erased (uint8_t page_id) {
    const uint16_t erased = 0xFFFF;
    void *cur_addr = flash_get_address(page_id);
    void *end_addr = cur_addr + FLASH_PAGE_LEN;
    for (; cur_addr < end_addr; cur_addr += 2) {
        if (flash_read16(cur_addr) != erased) {
            return false;
        }
    }
    return true;
}
