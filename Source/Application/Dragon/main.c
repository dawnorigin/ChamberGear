/* Library includes. */
#include "stm32f10x.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "ApplicationConfig.h"

extern void init_tasks(void);

void BSP_init(void);
void init_LED_task(void);

int main(void)
{
  /* Fetch BSP Initialization */
  BSP_init();

  /* Init LED flash task */
  init_LED_task();

  /* Initialize application tasks */
  init_tasks();

  /* Start the scheduler. */
  vTaskStartScheduler();

  /* Will only get here if there was not enough heap space to create the
  idle task. */
  return 0;
}

static portTASK_FUNCTION( vLEDFlashTask, pvParameters ) {
  for(;;)
  {
    GPIO_SetBits(GPIOC, GPIO_Pin_1);
    vTaskDelay(500);
    GPIO_ResetBits(GPIOC, GPIO_Pin_1);
    vTaskDelay(500);
  }
}

void init_LED_task(void) {  
  xTaskCreate( vLEDFlashTask, 
              "LEDFlash", 
              configMINIMAL_STACK_SIZE, 
              NULL, 
              tskIDLE_PRIORITY + 1, 
              ( TaskHandle_t * ) NULL );
}

void BSP_init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* Enable clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | 
                         RCC_APB2Periph_GPIOA | 
                         RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC |
                         RCC_APB2Periph_GPIOD |
                         RCC_APB2Periph_GPIOE, ENABLE);  
  
  /* SWJ remap: disable JTAG, enable SWO */
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
  
#ifdef USE_STDPERIPH_DRIVER
  /* Configure the NVIC Preemption Priority Bits */  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
#endif
  
  /* Configure LED(PA7) GPIO */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}


#if ( configUSE_IDLE_HOOK == 1 )
void vApplicationIdleHook(void) {
}
#endif