/*

  _    _ _______   __  __ _____ _____ _____   ____  _   _
 | |  | |__   __| |  \/  |_   _/ ____|  __ \ / __ \| \ | |
 | |__| |  | |    | \  / | | || |    | |__) | |  | |  \| |
 |  __  |  | |    | |\/| | | || |    |  _  /| |  | | . ` |
 | |  | |  | |    | |  | |_| || |____| | \ \| |__| | |\  |
 |_|  |_|  |_|    |_|  |_|_____\_____|_|  \_\\____/|_| \_|
 =================== Advanced R&D ========================

 Copyright (c) 2023 HT Micron Semicondutores S.A.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

/*!
 * \file HT_Fsm.h
 * \brief MQTT Example finite state machine. 
  * \author HT Micron Advanced R&D,
 *         Hêndrick Bataglin Gonçalves, Christian Roberto Lehmen,  Matheus da Silva Zorzeto, Felipe Kalinski Ferreira,
 *         Leandro Borges, Mauricio Carlotto Ribeiro, Henrique Kuhn, Cleber Haack, Eduardo Mendel
 *         Gleiser Alvarez Arrojo
 * 
 * \link https://github.com/htmicron
 * \version 0.1
 * \date February 23, 2023
 */

#ifndef __HT_SENSECLIMA_H__
#define __HT_SENSECLIMA_H__

#include "stdint.h"
#include "main.h"
#include "HT_MQTT_Api.h"
#include "MQTTFreeRTOS.h"
#include "bsp.h"
#include "HT_GPIO_Api.h"
#include "cmsis_os2.h"
#include "MQTTClient.h"

/* Defines  ------------------------------------------------------------------*/
#define LED_TASK_STACK_SIZE  (1024*4) 
#define HT_MQTT_KEEP_ALIVE_INTERVAL 240                   /**</ Keep alive interval in ms. */
#define HT_MQTT_VERSION 4                                 /**</ MQTT protocol version. */

#if MQTT_TLS_ENABLE == 1
#define HT_MQTT_PORT   8883                               /**</ MQTT TCP TLS port. */
#else
#define HT_MQTT_PORT   1883                               /**</ MQTT TCP port. */
#endif

#define HT_MQTT_SEND_TIMEOUT 60000                        /**</ MQTT TX timeout. */
#define HT_MQTT_RECEIVE_TIMEOUT   60000                   /**</ MQTT RX timeout. */
#define HT_MQTT_BUFFER_SIZE 1024                          /**</ Maximum MQTT buffer size. */
#define HT_SUBSCRIBE_BUFF_SIZE  6                         /**</ Maximum buffer size to received from MQTT subscribe. */

/* Typedefs  ------------------------------------------------------------------*/

/**
 * \enum HT_ConnectionStatus
 * \brief HTNB32L-XXX connection status.
 */
typedef enum {
    HT_CONNECTED = 0,
    HT_NOT_CONNECTED
} HT_ConnectionStatus;

/**
 * \enum HT_FSM_States
 * \brief States definition for the FSM.
 */
typedef enum {
    HT_WAIT_FOR_BUTTON_STATE = 0,
    HT_PUSH_BUTTON_HANDLE_STATE,
    HT_MQTT_SUBSCRIBE_STATE,
    HT_MQTT_PUBLISH_STATE,
    HT_CHECK_SOCKET_STATE,
    HT_SUBSCRIBE_HANDLE_STATE
} HT_FSM_States;

/**
 * \enum HT_Button
 * \brief Available buttons for MQTT Example app.
 */
typedef enum {
    HT_BLUE_BUTTON = 0,
    HT_WHITE_BUTTON,
    HT_UNDEFINED
} HT_Button;

/* Functions ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn void HT_FSM_SetSubscribeBuff(uint8_t *buff, uint8_t payload_len)
 * \brief Stores data received from subscribe in a usable buffer.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
void HT_FSM_SetSubscribeBuff(uint8_t *buff, uint8_t payload_len);
void led_state_manager(uint8_t *payload, uint8_t payload_len, 
    uint8_t *topic, uint8_t topic_len);

/*!******************************************************************
 * \fn void HT_Fsm(void)
 * \brief Finite State Machine of Push Button Example. Connect to
 *        MQTT Broker, then subscribe to MQTT topic. Waits for button
 *        interruption and publish the color of the button pressed.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
void HT_Fsm(void);

#endif /* __HT_SENSECLIMA_H__ */

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
