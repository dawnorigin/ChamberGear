/* Includes ------------------------------------------------------------------*/
/* Library includes. */
#include "stm32f10x.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "ApplicationConfig.h"
#include "player.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
enum {
  INIT,
  IDLE,
  PROCESS,
  ACTION,
  FINISH
};
/* Private macro -------------------------------------------------------------*/
#define CRYSTAL_STACK_SIZE    (configMINIMAL_STACK_SIZE * 2)
#define CRYSTAL_PRIORITY			(tskIDLE_PRIORITY + 1)

#define HALL_PORT             (GPIOA)
#define HALL_PIN              (GPIO_Pin_0)
#define HALL_STATUS()         (GPIO_ReadInputDataBit(HALL_PORT,HALL_PIN))
#define ENABLE_HALL_IT()      {EXTI->IMR |= HALL_PIN;}
#define DISABLE_HALL_IT()     {EXTI->IMR &= ~ HALL_PIN;}

#define PLAYER_BUSY_PORT      (GPIOA)
#define PLAYER_BUSY_PIN       (GPIO_Pin_3)
#define PLAYER_BUSY_STATUS()  (GPIO_ReadInputDataBit(PLAYER_BUSY_PORT, \
                                                     PLAYER_BUSY_PIN))

#define LAMP_PORT             (GPIOA)
#define LAMP_PIN              (GPIO_Pin_1)
#define LAMP_OFF()            {GPIO_SetBits(LAMP_PORT, LAMP_PIN);}
#define LAMP_ON()             {GPIO_ResetBits(LAMP_PORT, LAMP_PIN);}

#define DOOR_PORT             (GPIOA)
#define DOOR_PIN              (GPIO_Pin_2)
#define DOOR_OFF()            {GPIO_SetBits(DOOR_PORT, DOOR_PIN);}
#define DOOR_ON()             {GPIO_ResetBits(DOOR_PORT, DOOR_PIN);}

/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xHallSemaphore = NULL;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION(vCrystalTask, pvParameters) {
  int status = INIT;

	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        /* Turn off the lamps */
        LAMP_OFF();
        /* Close the door */
        DOOR_OFF();
        /* Disable hall interrupt */
        DISABLE_HALL_IT();
        /* Clean the xHallSemaphore semaphore */
        while (pdTRUE == xSemaphoreTake(xHallSemaphore, 0));
        /* Enable hall interrupt */
        ENABLE_HALL_IT();
        status = IDLE;
        break;
      }
      case IDLE: {
        /* Occur when the sword inserts the crystal stage */        
        if (xSemaphoreTake(xHallSemaphore, portMAX_DELAY) == pdTRUE) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          /* Check the hall switch status */
          if (Bit_RESET == HALL_STATUS()) {
            status = ACTION;
          } else {
            ENABLE_HALL_IT();
          }
        }
        break;
      }
      case ACTION: {
        /* Play music */
        player_play_file(FILE_1, 0);
        /* Turn on the lamps */
        LAMP_ON();
        while(Bit_RESET != PLAYER_BUSY_STATUS());
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        DOOR_ON();
        status = FINISH;
        break;
      }
      case FINISH: {
        /* Reset logic */
        if (xSemaphoreTake(xResetSemaphore, portMAX_DELAY) == pdTRUE) {
          status = INIT;
        }
        break;
      }
    }
  }
}

static void hardware_init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  
  /* Configure GPIO */
  GPIO_InitStructure.GPIO_Pin = HALL_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(HALL_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PLAYER_BUSY_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PLAYER_BUSY_PORT, &GPIO_InitStructure);
  
  GPIO_SetBits(LAMP_PORT, LAMP_PIN);
  GPIO_InitStructure.GPIO_Pin = LAMP_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(LAMP_PORT, &GPIO_InitStructure);
  
  GPIO_SetBits(DOOR_PORT, DOOR_PIN);
  GPIO_InitStructure.GPIO_Pin = DOOR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(DOOR_PORT, &GPIO_InitStructure);
  
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
  
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);  
  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                          configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable USART clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  /* Configure USART1 Tx (PA.09) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure USART1 Rx (PA.10) as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);  
  
  /* USART configured using USART_StructInit():
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
  USART_StructInit(&USART_InitStructure);
  
  /* Configure USART1 */
  USART_Init(USART1, &USART_InitStructure);
  
  USART_ITConfig( USART1, USART_IT_RXNE, ENABLE );

  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                                  configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init( &NVIC_InitStructure );

  USART_Cmd( USART1, ENABLE );
}

void init_tasks(void) {
  /* Initialize hardware */
  hardware_init();

  /* Create reset semaphore */
  xResetSemaphore = xSemaphoreCreateBinary();
  
  xHallSemaphore = xSemaphoreCreateBinary();
  xTaskCreate( vCrystalTask, 
              "Crystal", 
              CRYSTAL_STACK_SIZE, 
              NULL, 
              CRYSTAL_PRIORITY, 
              ( TaskHandle_t * ) NULL );
}

