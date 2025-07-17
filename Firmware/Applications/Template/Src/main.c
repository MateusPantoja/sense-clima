//#include "string.h"
//#include "bsp.h"
//#include "ostask.h"
//#include "debug_log.h"
//#include "FreeRTOS.h"
#include "main.h"
#include "ps_lib_api.h"
#include "flash_qcx212.h"

static StaticTask_t initTask;
static uint8_t appTaskStack[INIT_TASK_STACK_SIZE];
static volatile uint32_t Event;
static QueueHandle_t psEventQueueHandle;
static uint8_t gImsi[16] = {0};
static uint32_t gCellID = 0;
static NmAtiSyncRet gNetworkInfo;

uint8_t mqttEpSlpHandler = 0xff;

static volatile uint8_t simReady = 0;

static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | 
                                ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;



static void HT_SetConnectioParameters(void) {
    uint8_t cid = 0;
    PsAPNSetting apnSetting;
    int32_t ret;
    uint8_t networkMode = 0; //nb-iot network mode
    uint8_t bandNum = 1;
    uint8_t band = 28;

    ret = appSetBandModeSync(networkMode, bandNum, &band);
    if(ret == CMS_RET_SUCC) {
        printf("SetBand Result: %d\n", ret);
    }

    apnSetting.cid = 0;
    apnSetting.apnLength = strlen("iot.datatem.com.br");
    strcpy((char *)apnSetting.apnStr, "iot.datatem.com.br");
    apnSetting.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;
    ret = appSetAPNSettingSync(&apnSetting, &cid);
}

static void sendQueueMsg(uint32_t msgId, uint32_t xTickstoWait) {
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle)
    {
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait))
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask80, P_INFO, 0, "xQueueSend error");
        }
    }
}

static INT32 registerPSUrcCallback(urcID_t eventID, void *param, uint32_t paramLen) {
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *cereg = NULL;
    UINT8 rssi = 0;
    NmAtiNetifInfo *netif = NULL;

    switch(eventID)
    {
        case NB_URC_ID_SIM_READY:
        {
            imsi = (CmiSimImsiStr *)param;
            memcpy(gImsi, imsi->contents, imsi->length);
            simReady = 1;
            break;
        }
        case NB_URC_ID_MM_SIGQ:
        {
            rssi = *(UINT8 *)param;
            HT_TRACE(UNILOG_MQTT, mqttAppTask81, P_INFO, 1, "RSSI signal=%d", rssi);
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED:
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask82, P_INFO, 0, "Default bearer activated");
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED:
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask83, P_INFO, 0, "Default bearer Deactivated");
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED:
        {
            cereg = (CmiPsCeregInd *)param;
            gCellID = cereg->celId;
            HT_TRACE(UNILOG_MQTT, mqttAppTask84, P_INFO, 4, "CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);
            break;
        }
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
                sendQueueMsg(QMSG_ID_NW_IPV4_READY, 0);
            break;
        }

        default:
            break;
    }
    return 0;
}

static void HT_MQTTExampleTask(void *arg){
    
    int32_t ret;
    uint8_t psmMode = 0, actType = 0;
    uint16_t tac = 0;
    uint32_t tauTime = 0, activeTime = 0, cellID = 0, nwEdrxValueMs = 0, nwPtwMs = 0;

    eventCallbackMessage_t *queueItem = NULL;

    registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback);
    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
    if (psEventQueueHandle == NULL)
    {
        HT_TRACE(UNILOG_MQTT, mqttAppTask0, P_INFO, 0, "psEventQueue create error!");
        return;
    }

    slpManApplyPlatVoteHandle("EP_MQTT",&mqttEpSlpHandler);
    slpManPlatVoteDisableSleep(mqttEpSlpHandler, SLP_ACTIVE_STATE); //SLP_SLP2_STATE 
    HT_TRACE(UNILOG_MQTT, mqttAppTask1, P_INFO, 0, "first time run mqtt example");

    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    printf("HTNB32L-XXX-Template SenseClima Device!\n");
    printf("Trying to connect...\n");
    while(!simReady);
    HT_SetConnectioParameters();

    while (1)
    {
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IPV4_READY:
                case QMSG_ID_NW_IPV6_READY:
                case QMSG_ID_NW_IPV4_6_READY:
                    appGetImsiNumSync((CHAR *)gImsi);
                    HT_STRING(UNILOG_MQTT, mqttAppTask2, P_SIG, "IMSI = %s", gImsi);
                
                    appGetNetInfoSync(gCellID, &gNetworkInfo);
                    if ( NM_NET_TYPE_IPV4 == gNetworkInfo.body.netInfoRet.netifInfo.ipType)
                        HT_TRACE(UNILOG_MQTT, mqttAppTask3, P_INFO, 4,"IP:\"%u.%u.%u.%u\"", ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[0],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[1],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[2],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[3]);
                    ret = appGetLocationInfoSync(&tac, &cellID);
                    HT_TRACE(UNILOG_MQTT, mqttAppTask4, P_INFO, 3, "tac=%d, cellID=%d ret=%d", tac, cellID, ret);
                    //edrxModeValue = CMI_MM_ENABLE_EDRX_AND_ENABLE_IND;
                    actType = CMI_MM_EDRX_NB_IOT;
                    //reqEdrxValueMs = 20480;
                    // appSetEDRXSettingSync(edrxModeValue, actType, reqEdrxValueMs);
                    ret = appGetEDRXSettingSync(&actType, &nwEdrxValueMs, &nwPtwMs);
                    HT_TRACE(UNILOG_MQTT, mqttAppTask5, P_INFO, 4, "actType=%d, nwEdrxValueMs=%d nwPtwMs=%d ret=%d", actType, nwEdrxValueMs, nwPtwMs, ret);

                    psmMode = 1;
                    tauTime = 4000;
                    activeTime = 30;

                    {
                        appGetPSMSettingSync(&psmMode, &tauTime, &activeTime);
                        HT_TRACE(UNILOG_MQTT, mqttAppTask6, P_INFO, 3, "Get PSM info mode=%d, TAU=%d, ActiveTime=%d", psmMode, tauTime, activeTime);
                    }

                    HT_Fsm();
               
                    break;
                case QMSG_ID_NW_DISCONNECT:
                    break;

                default:
                    break;
            }
            free(queueItem);
        }
    }

}

/**
  \fn          static void appInit(void *arg)
  \brief
  \return
*/
static void appInit(void *arg)
{
    osThreadAttr_t task_attr;
    
    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_CONTROL) != 0)
        HAL_UART_RecvFlowControl(false);
    
    memset(&task_attr,0,sizeof(task_attr));
    memset(appTaskStack, 0xA5,INIT_TASK_STACK_SIZE);
    task_attr.name = "HT_MQTTExample";
    task_attr.stack_mem = appTaskStack;
    task_attr.stack_size = INIT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &initTask;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(HT_MQTTExampleTask, NULL, &task_attr);

}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void)
{
    BSP_CommonInit();
    osKernelInitialize();

    setvbuf(stdout, NULL, _IONBF, 0);
    
    registerAppEntry(appInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);

}
