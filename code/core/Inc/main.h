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
    #define BUZZER_GPIO_Port GPIOB  ///< Порт GPIO линии BUZZER
    #define BUZZER_GPIO_Pin LL_GPIO_PIN_9  ///< Маска пина линии BUZZER
    /**@} */

    /**
     * @addtogroup KEYBOARD_PIN_DESCRIPTION 
     * @{
     */
    #define BUTTON_UP_GPIO_Port GPIOA  ///< Порт GPIO линии BUTTON_UP
    #define BUTTON_UP_GPIO_Pin LL_GPIO_PIN_0  ///< Маска пина линии BUTTON_UP
    #define BUTTON_REJECT_GPIO_Port GPIOA  ///< Порт GPIO линии BUTTON_REJECT
    #define BUTTON_REJECT_GPIO_Pin LL_GPIO_PIN_1  ///< Маска пина линии BUTTON_REJECT
    #define BUTTON_DOWN_GPIO_Port GPIOA  ///< Порт GPIO линии BUTTON_DOWN
    #define BUTTON_DOWN_GPIO_Pin LL_GPIO_PIN_2  ///< Маска пина линии BUTTON_DOWN
    #define BUTTON_ACCEPT_GPIO_Port GPIOA  ///< Порт GPIO линии BUTTON_ACCEPT
    #define BUTTON_ACCEPT_GPIO_Pin LL_GPIO_PIN_3  ///< Маска пина линии BUTTON_ACCEPT
    /** @} */

    /**
     * @addtogroup INA239_PIN_DESCRIPTION 
     * @{
     */
    #define nCS_MON_GPIO_Port GPIOA  ///< Порт GPIO линии nCS_MON
    #define nCS_MON_GPIO_Pin LL_GPIO_PIN_15  ///< Маска пина линии nCS_MON
    #define nALERT_GPIO_Port GPIOB  ///< Порт GPIO линии nALERT
    #define nALERT_GPIO_Pin LL_GPIO_PIN_12  ///< Маска пина линии nALERT
    /**@} */

    /**
     * @addtogroup LCD_PIN_DESCRIPTION 
     * @{
     */
    #define nCS_LCD_GPIO_Port GPIOB  ///< Порт GPIO линии nCS_LCD
    #define nCS_LCD_GPIO_Pin LL_GPIO_PIN_6  ///< Маска пина линии nCS_LCD
    #define LCD_ADDR0_GPIO_Port GPIOB  ///< Порт GPIO линии LCD_ADDR0
    #define LCD_ADDR0_GPIO_Pin LL_GPIO_PIN_7  ///< Маска пина линии LCD_ADDR0
    #define nRESET_LCD_GPIO_Port GPIOB  ///< Порт GPIO линии nRESET_LCD
    #define nRESET_LCD_GPIO_Pin LL_GPIO_PIN_8  ///< Маска пина линии nRESET_LCD
    /**@} */

    /**
     * @addtogroup SPI2_PIN_DESCRIPTION 
     * @{
     */
    #define SPI2_SCK_GPIO_Port GPIOB  ///< Порт GPIO линии SPI2_SCK
    #define SPI2_SCK_GPIO_Pin LL_GPIO_PIN_13  ///< Маска пина линии SPI2_SCK
    #define SPI2_MISO_GPIO_Port GPIOB  ///< Порт GPIO линии SPI2_MISO
    #define SPI2_MISO_GPIO_Pin LL_GPIO_PIN_14  ///< Маска пина линии SPI2_MISO
    #define SPI2_MOSI_GPIO_Port GPIOB  ///< Порт GPIO линии SPI2_MOSI
    #define SPI2_MOSI_GPIO_Pin LL_GPIO_PIN_15  ///< Маска пина линии SPI2_MOSI
    /**@} */

    /**
     * @addtogroup HV_GEN_PIN_DESCRIPTION
     * @{
     */
    #define HV_GEN_PWM_GPIO_Port GPIOA  ///< Порт GPIO линии HV_GEN_PWM
    #define HV_GEN_PWM_GPIO_Pin LL_GPIO_PIN_8  ///< Маска пина линии HV_GEN_PWM
    /**@} */

    /**
     * @addtogroup USB_PIN_DESCRIPTION
     * @{
     */
    #define USB_DP_GPIO_Port GPIOA  ///< Порт GPIO линии USB_DP
    #define USB_DP_GPIO_Pin LL_GPIO_PIN_11  ///< Маска пина линии USB_DP
    #define USB_DN_GPIO_Port GPIOA  ///< Порт GPIO линии USB_DN
    #define USB_DN_GPIO_Pin LL_GPIO_PIN_12  ///< Маска пина линии USB_DN
    /**@} */

    /**
     * @addtogroup MUX_PIN_DESCRIPTION
     * @{
     */
        /**
         * @addtogroup MUX1_PIN_DESCRIPTION
         * @{
         */
        #define nMUX1_EN_GPIO_Port GPIOB  ///< Порт GPIO линии nMUX1_EN
        #define nMUX1_EN_GPIO_Pin LL_GPIO_PIN_0  ///< Маска пина линии nMUX1_EN
        #define MUX1_A_GPIO_Port GPIOB  ///< Порт GPIO линии MUX1_A
        #define MUX1_A_GPIO_Pin LL_GPIO_PIN_10  ///< Маска пина линии MUX1_A
        #define MUX1_B_GPIO_Port GPIOB  ///< Порт GPIO линии MUX1_B
        #define MUX1_B_GPIO_Pin LL_GPIO_PIN_11  ///< Маска пина линии MUX1_B
        #define MUX1_C_GPIO_Port GPIOA  ///< Порт GPIO линии MUX1_C
        #define MUX1_C_GPIO_Pin LL_GPIO_PIN_10  ///< Маска пина линии MUX1_C
        /**@} */

        /**
         * @addtogroup MUX2_PIN_DESCRIPTION 
         * @{
         */
        #define nMUX2_EN_GPIO_Port GPIOB  ///< Порт GPIO линии nMUX2_EN
        #define nMUX2_EN_GPIO_Pin LL_GPIO_PIN_1  ///< Маска пина линии nMUX2_EN
        #define MUX2_A_GPIO_Port GPIOB  ///< Порт GPIO линии MUX2_A
        #define MUX2_A_GPIO_Pin LL_GPIO_PIN_3  ///< Маска пина линии MUX2_A
        #define MUX2_B_GPIO_Port GPIOB  ///< Порт GPIO линии MUX2_B
        #define MUX2_B_GPIO_Pin LL_GPIO_PIN_4  ///< Маска пина линии MUX2_B
        #define MUX2_C_GPIO_Port GPIOB  ///< Порт GPIO линии MUX2_C
        #define MUX2_C_GPIO_Pin LL_GPIO_PIN_5  ///< Маска пина линии MUX2_C
        /**@} */
    /**@} */

    /**
     * @addtogroup RJ45_PIN_DESCRIPTION 
     * @{
     */
    #define RJ_SHIELD_P_GPIO_Port GPIOA  ///< Порт GPIO линии RJ_SHIELD_P
    #define RJ_SHIELD_P_GPIO_Pin LL_GPIO_PIN_5  ///< Маска пина линии RJ_SHIELD_P
    #define RJ_SHIELD_N_GPIO_Port GPIOA  ///< Порт GPIO линии RJ_SHIELD_N
    #define RJ_SHIELD_N_GPIO_Pin LL_GPIO_PIN_9  ///< Маска пина линии RJ_SHIELD_N
    /** @} */

    /**
     * @addtogroup GEN_DESTINATION_PIN_DESCRIPTION 
     * @{
     */
    #define PROBE_EN_GPIO_Port GPIOA  ///< Порт GPIO линии PROBE_EN
    #define PROBE_EN_GPIO_Pin LL_GPIO_PIN_6  ///< Маска пина линии PROBE_EN
    #define DSUB_EN_GPIO_Port GPIOA  ///< Порт GPIO линии DSUB_EN
    #define DSUB_EN_GPIO_Pin LL_GPIO_PIN_7  ///< Маска пина линии DSUB_EN
    /**@} */

/**@} */

extern uint32_t SystemCoreClock;

void Error_Handler (void);

#endif /* MAIN_H_ */