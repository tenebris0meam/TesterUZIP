#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f103xb.h"
#include "stm32f1xx_ll_gpio.h"

/**
 * @addtogroup PIN_DESCRIPTIONS
 * @{
 */
    /**
     * @addtogroup BUZZER_PIN_DESCRIPTION 
     * @{
     */
    #define BUZZER_GPIO_Port GPIOB
    #define BUZZER_GPIO_Pin LL_GPIO_PIN_9
    /**@} */

    /**
     * @addtogroup KEYBOARD_PIN_DESCRIPTION 
     * @{
     */
    #define BUTTON_UP_GPIO_Port GPIOA
    #define BUTTON_UP_GPIO_Pin LL_GPIO_PIN_0
    #define BUTTON_REJECT_GPIO_Port GPIOA
    #define BUTTON_REJECT_GPIO_Pin LL_GPIO_PIN_1
    #define BUTTON_DOWN_GPIO_Port GPIOA
    #define BUTTON_DOWN_GPIO_Pin LL_GPIO_PIN_2
    #define BUTTON_ACCEPT_GPIO_Port GPIOA
    #define BUTTON_ACCEPT_GPIO_Pin LL_GPIO_PIN_3
    /** @} */

    /**
     * @addtogroup INA239_PIN_DESCRIPTION 
     * @{
     */
    #define nCS_MON_GPIO_Port GPIOA
    #define nCS_MON_GPIO_Pin LL_GPIO_PIN_15
    #define nALERT_GPIO_Port GPIOB
    #define nALERT_GPIO_Pin LL_GPIO_PIN_12
    /**@} */

    /**
     * @addtogroup LCD_PIN_DESCRIPTION 
     * @{
     */
    #define nCS_LCD_GPIO_Port GPIOB
    #define nCS_LCD_GPIO_Pin LL_GPIO_PIN_6
    #define LCD_ADDR0_GPIO_Port GPIOB
    #define LCD_ADDR0_GPIO_Pin LL_GPIO_PIN_7
    #define nRESET_LCD_GPIO_Port GPIOB
    #define nRESET_LCD_GPIO_Pin LL_GPIO_PIN_8
    /**@} */

    /**
     * @addtogroup SPI2_PIN_DESCRIPTION 
     * @{
     */
    #define SPI2_SCK_GPIO_Port GPIOB
    #define SPI2_SCK_GPIO_Pin LL_GPIO_PIN_13
    #define SPI2_MISO_GPIO_Port GPIOB
    #define SPI2_MISO_GPIO_Pin LL_GPIO_PIN_14
    #define SPI2_MOSI_GPIO_Port GPIOB
    #define SPI2_MOSI_GPIO_Pin LL_GPIO_PIN_15
    /**@} */

    /**
     * @addtogroup HV_GEN_PIN_DESCRIPTION
     * @{
     */
    #define HV_GEN_PWM_GPIO_Port GPIOA
    #define HV_GEN_PWM_GPIO_Pin LL_GPIO_PIN_8
    /**@} */

    /**
     * @addtogroup USB_PIN_DESCRIPTION
     * @{
     */
    #define USB_DP_GPIO_Port GPIOA
    #define USB_DP_GPIO_Pin LL_GPIO_PIN_11
    #define USB_DN_GPIO_Port GPIOA
    #define USB_DN_GPIO_Pin LL_GPIO_PIN_12
    /**@} */

    /**
     * @addtogroup MUX_PIN_DESCRIPTION
     * @{
     */
        /**
         * @addtogroup MUX1_PIN_DESCRIPTION
         * @{
         */
        #define nMUX1_EN_GPIO_Port GPIOB
        #define nMUX1_EN_GPIO_Pin LL_GPIO_PIN_0
        #define MUX1_A_GPIO_Port GPIOB
        #define MUX1_A_GPIO_Pin LL_GPIO_PIN_10
        #define MUX1_B_GPIO_Port GPIOB
        #define MUX1_B_GPIO_Pin LL_GPIO_PIN_11
        #define MUX1_C_GPIO_Port GPIOA
        #define MUX1_C_GPIO_Pin LL_GPIO_PIN_10
        /**@} */

        /**
         * @addtogroup MUX2_PIN_DESCRIPTION 
         * @{
         */
        #define nMUX2_EN_GPIO_Port GPIOB
        #define nMUX2_EN_GPIO_Pin LL_GPIO_PIN_1
        #define MUX2_A_GPIO_Port GPIOB
        #define MUX2_A_GPIO_Pin LL_GPIO_PIN_3
        #define MUX2_B_GPIO_Port GPIOB
        #define MUX2_B_GPIO_Pin LL_GPIO_PIN_4
        #define MUX2_C_GPIO_Port GPIOB
        #define MUX2_C_GPIO_Pin LL_GPIO_PIN_5
        /**@} */
    /**@} */

    /**
     * @addtogroup RJ45_PIN_DESCRIPTION 
     * @{
     */
    #define RJ_SHIELD_P_GPIO_Port GPIOA
    #define RJ_SHIELD_P_GPIO_Pin LL_GPIO_PIN_5
    #define RJ_SHIELD_N_GPIO_Port GPIOA
    #define RJ_SHIELD_N_GPIO_Pin LL_GPIO_PIN_9
    /** @} */

    /**
     * @addtogroup GEN_DESTINATION_PIN_DESCRIPTION 
     * @{
     */
    #define PROBE_EN_GPIO_Port GPIOA
    #define PROBE_EN_GPIO_Pin LL_GPIO_PIN_6
    #define DSUB_EN_GPIO_Port GPIOA
    #define DSUB_EN_GPIO_Pin LL_GPIO_PIN_7
    /**@} */

/**@} */

extern uint32_t SystemCoreClock;

void Error_Handler (void);

#endif /* MAIN_H_ */