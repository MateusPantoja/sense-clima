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

#include "HT_SenseClima.h"


/* Function prototypes  ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn static void HT_YieldThread(void *arg)
 * \brief Thread created as MQTT background.
 *
 * \param[in]  void *arg                    Thread parameter.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_YieldThread(void *arg);

/*!******************************************************************
 * \fn static void HT_Yield_Thread(void *arg)
 * \brief Creates a thread that will be the MQTT background.
 *
 * \param[in]  void *arg                    Parameters that will be used in the created thread.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_Yield_Thread(void *arg);

/*!******************************************************************
 * \fn static void HT_FSM_MQTTWritePayload(uint8_t *ptr, uint8_t size)
 * \brief Copy the *ptr content to the mqtt_payload.
 *
 * \param[in]  uint8_t *ptr                 Pointer with the content that will be copied.
 * \param[in]  uint8_t size                 Buffer size.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_MQTTWritePayload(uint8_t *ptr, uint8_t size);

/*!******************************************************************
 * \fn static void HT_FSM_LedStatus(HT_Led_Type led, uint16_t state)
 * \brief Change a specific led status to ON/OFF.
 *
 * \param[in]  HT_Led_Type led              LED id.
 * \param[in]  uint16_t state               LED state (ON/OFF)
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_LedStatus(HT_Led_Type led, uint16_t state);

/*!******************************************************************
 * \fn static HT_ConnectionStatus HT_FSM_MQTTConnect(void)
 * \brief Connects the device to the MQTT Broker and returns the connection
 * status.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval Connection status.
 *******************************************************************/
static HT_ConnectionStatus HT_FSM_MQTTConnect(void);


/* ---------------------------------------------------------------------------------------*/

static MQTTClient mqttClient;
static Network mqttNetwork;

//Buffer that will be published.
static uint8_t mqtt_payload[128] = {"Undefined Button"};
static uint8_t mqttSendbuf[HT_MQTT_BUFFER_SIZE] = {0};
static uint8_t mqttReadbuf[HT_MQTT_BUFFER_SIZE] = {0};

static const char clientID[] = {"SIP_HTNB32L-XXX"};
static const char username[] = {""};
static const char password[] = {""};

//MQTT broker host address
static const char addr[] = {"131.255.82.115"};
static char topic[25] = {0};


static const char topic_temperature[] = {"hana/externo/senseclima/00001/temperature"};
static const char topic_humidity[] = {"hana/externo/senseclima/00001/humidity"};
static const char topic_interval[] = {"hana/externo/senseclima/00001/interval"};

//extern uint8_t mqttEpSlpHandler;

//FSM state.
volatile HT_FSM_States state = HT_WAIT_FOR_BUTTON_STATE;

//Buffer where the digital twin messages will be stored.
static uint8_t subscribe_buffer[HT_SUBSCRIBE_BUFF_SIZE] = {0};

static StaticTask_t yield_thread, dht_thread, sleep_thread;
static uint8_t yieldTaskStack[1024*4], dhtTaskStack[1024*4], sleepTaskStack[1024*2];

#define TIMER_ID        0
#define RTC_TIMEOUT_MS  60000  // 10 segundos

uint8_t voteHandle = 0xFF;
extern uint8_t mqttEpSlpHandler;


void beforeHibernateCb(void *pdata, slpManLpState state) {
    printf("[Callback] Antes de Hibernate\n");
}

void afterHibernateCb(void *pdata, slpManLpState state) {
    printf("[Callback] Acordou de Hibernate\n");
}

void sleepWithMode(slpManSlpState_t mode) {

    printf("\n=== Entrando em Modo Sono %d===\n", mode);

    appSetCFUN(0);
    appSetEcSIMSleepSync(1);

    slpManSetPmuSleepMode(true, mode, false);

    // 1. Setup dos modos
    slpManApplyPlatVoteHandle("SLEEP_TEST", &voteHandle);

    slpManRegisterUsrdefinedBackupCb(beforeHibernateCb, NULL, SLPMAN_HIBERNATE_STATE);
    slpManRegisterUsrdefinedRestoreCb(afterHibernateCb, NULL, SLPMAN_HIBERNATE_STATE);
    

    // Habilita o modo de sono
    slpManPlatVoteEnableSleep(voteHandle, mode);

    //slpManPlatVoteDisableSleep(mqttEpSlpHandler, SLP_STATE_MAX);

    // Ativa o temporizador RTC como wakeup
    slpManDeepSlpTimerStart(TIMER_ID, RTC_TIMEOUT_MS);

    // Espera passiva — o sistema deve entrar em sono automaticamente
    while (1) {
        printf("Hibernando ....");
        osDelay(1000);  // após o tempo, sistema acorda e mensagens são exibidas
    }
}


static void HT_YieldThread(void *arg) {
    while (1) {
        // Wait function for 10ms to check if some message arrived in subscribed topic
        MQTTYield(&mqttClient, 10);
    }
}

static void HT_Yield_Thread(void *arg) {
    osThreadAttr_t task_attr;

    memset(&task_attr,0,sizeof(task_attr));
    memset(yieldTaskStack, 0xA5,LED_TASK_STACK_SIZE);
    task_attr.name = "yield_thread";
    task_attr.stack_mem = yieldTaskStack;
    task_attr.stack_size = LED_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &yield_thread;
    task_attr.cb_size = sizeof(StaticTask_t);

    osThreadNew(HT_YieldThread, NULL, &task_attr);
}


static void HT_DhtThread(void *arg) {
        float temp, humi;
        char tempString[10], humString[10];
        int count = 0;

        while (1)
        {
           
            int dht_status = DHT22_Read(&temp, &humi);
            
            printf("\nExecultando contagem %d\n", count + 1);
            
            if(count >= 5) {
                printf("\nProcesso para deep sleep\n");
                sleepWithMode(SLP_HIB_STATE);
            }
            
            if(dht_status == 0){
                int temp_int = (int) (temp * 10);
                int hum_int = (int) (humi * 10);
                sprintf(tempString, "%d.%d", temp_int/10, temp_int%10);
                sprintf(humString, "%d.%d", hum_int/10, hum_int%10);
                printf("\n\nExecultando Dht\n");
                printf("\ntemp %s*C | hum %s %%\n", tempString, humString);

                while(!mqttClient.isconnected){
                    if(HT_FSM_MQTTConnect() == HT_NOT_CONNECTED) {
                        printf("\n MQTT Connection Error!\n");
                        osDelay(5000);
                    }
                }

                HT_MQTT_Publish(&mqttClient, (char *)topic_temperature, (uint8_t *)tempString, strlen(tempString), QOS0, 0, 0, 0);
                HT_MQTT_Publish(&mqttClient, (char *)topic_humidity, (uint8_t *)humString, strlen(humString), QOS0, 0, 0, 0);
            }

            count++;
            osDelay(1000);
     
        }
        
}

static void HT_Dht_Thread(void *arg) {
    osThreadAttr_t task_attr;

    memset(&task_attr,0,sizeof(task_attr));
    memset(dhtTaskStack, 0xA5,LED_TASK_STACK_SIZE);
    task_attr.name = "dht_thread";
    task_attr.stack_mem = dhtTaskStack;
    task_attr.stack_size = LED_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &dht_thread;
    task_attr.cb_size = sizeof(StaticTask_t);

    osThreadNew(HT_DhtThread, NULL, &task_attr);
}

static void HT_FSM_MQTTWritePayload(uint8_t *ptr, uint8_t size) {
    // Reset payload and writes the message
    memset(mqtt_payload, 0, sizeof(mqtt_payload));
    memcpy(mqtt_payload, ptr, size);
}


static HT_ConnectionStatus HT_FSM_MQTTConnect(void) {

    // Connect to MQTT Broker using client, network and parameters needded. 
    if(HT_MQTT_Connect(&mqttClient, &mqttNetwork, (char *)addr, HT_MQTT_PORT, HT_MQTT_SEND_TIMEOUT, HT_MQTT_RECEIVE_TIMEOUT,
                (char *)clientID, (char *)username, (char *)password, HT_MQTT_VERSION, HT_MQTT_KEEP_ALIVE_INTERVAL, mqttSendbuf, HT_MQTT_BUFFER_SIZE, mqttReadbuf, HT_MQTT_BUFFER_SIZE)) {
        return HT_NOT_CONNECTED;   
    }

    printf("MQTT Connection Success!\n");

    return HT_CONNECTED;
}

void HT_FSM_SetSubscribeBuff(uint8_t *buff, uint8_t payload_len) {
    memcpy(subscribe_buffer, buff, payload_len);
}

void led_state_manager(uint8_t *payload, uint8_t payload_len, 
    uint8_t *topic, uint8_t topic_len) {
   
        uint8_t state = atoi(payload);

}


void HT_Fsm(void) {

    // Initialize MQTT Client and Connect to MQTT Broker defined in global variables
    printf("\nTentando Conectar ao MQTT CLient...");
    if(HT_FSM_MQTTConnect() == HT_NOT_CONNECTED) {
        printf("\n MQTT Connection Error!\n");
        while(1);
    }

    
    // Init irqn after connection
    //HT_GPIO_ButtonInit();

    printf("\nIniciando DHT !!!\n");
    DHT22_Init();

    HT_Dht_Thread(NULL);
    
    //HT_MQTT_Subscribe(&mqttClient, topic_blueled_sw, QOS0);
    //HT_MQTT_Subscribe(&mqttClient, topic_whiteled_sw, QOS0);
    
    //HT_Yield_Thread(NULL);

    // Led to sinalize connection stablished
    //HT_LED_GreenLedTask(NULL);

    //Ldr Task
    //HT_LDR_Task(NULL);

    //Btn Task()
    //HT_Btn_Thread_Start(NULL);

    printf("Executing SenseClima\n");
    while(1){
        osDelay(100);
    }


}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
