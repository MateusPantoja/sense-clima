/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "HT_GPIO_Api.h"
#include "ic_qcx212.h"
#include "slpman_qcx212.h"
#include <stdio.h>
#include "string.h"



/* Function prototypes  ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn static void HT_GPIO_IRQnCallback(void)
 * \brief GPIO IRQn callback function.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
//static void HT_GPIO_IRQnCallback(void);

/* ---------------------------------------------------------------------------------------*/
/*
static void HT_GPIO_IRQnCallback(void) {

    //button_irqn = 1;

    if (GPIO_GetInterruptFlags(BLUE_BUTTON_INSTANCE) & BLUE_BUTTON_MASK) {
       // button_color = HT_BLUE_BUTTON;
        blue_irqn_mask = GPIO_SaveAndSetIRQMask(BLUE_BUTTON_INSTANCE);
        btn = 1;
        GPIO_ClearInterruptFlags(BLUE_BUTTON_INSTANCE, BLUE_BUTTON_MASK);
    
    } else if(GPIO_GetInterruptFlags(WHITE_BUTTON_INSTANCE) & WHITE_BUTTON_MASK) {

       // button_color = HT_WHITE_BUTTON;
        white_irqn_mask = GPIO_SaveAndSetIRQMask(WHITE_BUTTON_INSTANCE);
        btn=2;
        GPIO_ClearInterruptFlags(WHITE_BUTTON_INSTANCE, WHITE_BUTTON_MASK);
    }
    
    osMessageQueuePut(btnQueue, &btn, 0, 0);

}
*/

void HT_GPIO_WritePin(uint16_t pin, uint32_t instance, uint16_t value) {
    // Write the value in the GPIO pin
    GPIO_PinWrite(instance, 1 << pin, (value ? 1 << pin : 0)); 
}

void HT_GPIO_ButtonInit(void) {

    pad_config_t padConfig;
    gpio_pin_config_t config;

    PAD_GetDefaultConfig(&padConfig);

    padConfig.mux = BUTTON_PAD_ALT_FUNC;

    // Blue button config
    PAD_SetPinConfig(BLUE_BUTTON_PAD_ID, &padConfig);
    PAD_SetPinPullConfig(BLUE_BUTTON_PAD_ID, PAD_InternalPullUp);

    // White button config
    PAD_SetPinConfig(WHITE_BUTTON_PAD_ID, &padConfig);
    PAD_SetPinPullConfig(WHITE_BUTTON_PAD_ID, PAD_InternalPullUp);

    config.pinDirection = GPIO_DirectionInput;
    config.misc.interruptConfig = GPIO_InterruptDisabled; 
;

    GPIO_PinConfig(BLUE_BUTTON_INSTANCE, BLUE_BUTTON_PIN, &config);
    GPIO_PinConfig(WHITE_BUTTON_INSTANCE, WHITE_BUTTON_PIN, &config);

    // Set IQR vector and enable IRQ
    //XIC_SetVector(PXIC_Gpio_IRQn, HT_GPIO_IRQnCallback);
    //XIC_EnableIRQ(PXIC_Gpio_IRQn);
}

void HT_GPIO_LedInit(void) {
    pad_config_t padConfig;
    gpio_pin_config_t config;

    // Set alternate function to GPIO
    PAD_GetDefaultConfig(&padConfig);

    padConfig.mux = LED_PAD_ALT_FUNC;
    PAD_SetPinConfig(BLUE_LED_PAD_ID, &padConfig);
    PAD_SetPinConfig(WHITE_LED_PAD_ID, &padConfig);
    PAD_SetPinConfig(GREEN_LED_PAD_ID, &padConfig);

    // Configure GPIO direction to output
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 0;

    // Configure pins with right GPIOs
    GPIO_PinConfig(BLUE_LED_INSTANCE, BLUE_LED_PIN, &config);
    GPIO_PinConfig(WHITE_LED_INSTANCE, WHITE_LED_PIN, &config);
    GPIO_PinConfig(GREEN_LED_INSTANCE, GREEN_LED_PIN, &config);

    // Set IO Voltage to 3.3V
    slpManNormalIOVoltSet(IOVOLT_3_30V);

}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/