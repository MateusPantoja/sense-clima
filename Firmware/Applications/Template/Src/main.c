//#include "string.h"
//#include "bsp.h"
//#include "ostask.h"
//#include "debug_log.h"
#include "FreeRTOS.h"
#include "main.h"
#include "ps_lib_api.h"
#include "flash_qcx212.h"

static StaticTask_t initTask;
static uint8_t appTaskStack[INIT_TASK_STACK_SIZE];


static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | 
                                ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;

static void HT_MQTTExampleTask(void *arg){
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    printf("\n\nHTNB32L-XXX MQTT Example!\n");
    printf("Trying to connect...\n");
    printf("Mateus Pantoja!\n");
    while(1);
  
    
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
