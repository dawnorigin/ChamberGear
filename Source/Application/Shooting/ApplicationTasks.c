/* Includes ------------------------------------------------------------------*/
/* Library includes. */
#include "stm32f10x.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "ApplicationConfig.h"
#include "player.h"
/* Private define ------------------------------------------------------------*/
enum {
  INIT,
  IDLE,
  ERRORLED,
  ACTION,
  FINISH
};
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define TASKS_STACK_SIZE        configMINIMAL_STACK_SIZE * 2
#define TASKS_PRIORITY			    ( tskIDLE_PRIORITY + 1 )

#define RECIEVERS_PORT          (GPIOA)
#define RECIEVER1_PIN           (GPIO_Pin_1)
#define RECIEVER2_PIN           (GPIO_Pin_2)
#define RECIEVER3_PIN           (GPIO_Pin_3)
#define RECIEVER4_PIN           (GPIO_Pin_4)
#define RECIEVERS_PIN_ALL       (RECIEVER1_PIN|RECIEVER2_PIN| \
                                 RECIEVER3_PIN|RECIEVER4_PIN)
#define RECIEVER_STATUS(x)      GPIO_ReadInputDataBit(RECIEVERS_PORT,(x))
#define ENABLE_RECIEVER_IT(x)   {EXTI->IMR |= (x);}
#define DISABLE_RECIEVER_IT(x)  {EXTI->IMR &= ~(x);}

#define LOCK_PORT               (GPIOA)
#define LOCK_PIN                (GPIO_Pin_6)
#define LOCK_OFF()              {GPIO_SetBits(LOCK_PORT, LOCK_PIN);}
#define LOCK_ON()               {GPIO_ResetBits(LOCK_PORT, LOCK_PIN);}

#define USART1_PORT             (GPIOA)
#define USART1_TX_PIN           (GPIO_Pin_9)
#define USART1_RX_PIN           (GPIO_Pin_10)

#define LED_S1_PORT             (GPIOA)
#define LED_S1_1_PIN            (GPIO_Pin_12)
#define LED_S1_2_PIN            (GPIO_Pin_13)
#define LED_S1_3_PIN            (GPIO_Pin_14)
#define LED_S1_4_PIN            (GPIO_Pin_15)
#define LED_S1_PIN_ALL          (LED_S1_1_PIN|LED_S1_2_PIN| \
                                 LED_S1_3_PIN|LED_S1_4_PIN)
#define LED_S1_OFF(x)           GPIO_SetBits(LED_S1_PORT, (x))
#define LED_S1_ON(x)            GPIO_ResetBits(LED_S1_PORT, (x))

#define LED_S2_PORT             (GPIOB)
#define LED_S2_1_PIN            (GPIO_Pin_0)
#define LED_S2_2_PIN            (GPIO_Pin_1)
#define LED_S2_3_PIN            (GPIO_Pin_2)
#define LED_S2_4_PIN            (GPIO_Pin_3)
#define LED_S2_PIN_ALL          (LED_S2_1_PIN|LED_S2_2_PIN| \
                                 LED_S2_3_PIN|LED_S2_4_PIN)
#define LED_S2_OFF(x)           GPIO_SetBits(LED_S2_PORT, (x))
#define LED_S2_ON(x)            GPIO_ResetBits(LED_S2_PORT, (x))

#define LED_S3_PORT             (GPIOB)
#define LED_S3_1_PIN            (GPIO_Pin_4)
#define LED_S3_2_PIN            (GPIO_Pin_5)
#define LED_S3_3_PIN            (GPIO_Pin_6)
#define LED_S3_4_PIN            (GPIO_Pin_7)
#define LED_S3_PIN_ALL          (LED_S3_1_PIN|LED_S3_2_PIN| \
                                 LED_S3_3_PIN|LED_S3_4_PIN)
#define LED_S3_OFF(x)           GPIO_SetBits(LED_S3_PORT, (x))
#define LED_S3_ON(x)            GPIO_ResetBits(LED_S3_PORT, (x))

#define LED_C1_PORT             (GPIOB)
#define LED_C1_1_PIN            (GPIO_Pin_8)
#define LED_C1_2_PIN            (GPIO_Pin_9)
#define LED_C1_3_PIN            (GPIO_Pin_10)
#define LED_C1_4_PIN            (GPIO_Pin_11)
#define LED_C1_5_PIN            (GPIO_Pin_12)
#define LED_C1_6_PIN            (GPIO_Pin_13)
#define LED_C1_PIN_ALL          (LED_C1_1_PIN|LED_C1_2_PIN|LED_C1_3_PIN| \
                                 LED_C1_4_PIN|LED_C1_5_PIN|LED_C1_6_PIN)
#define LED_C1_OFF(x)           GPIO_SetBits(LED_C1_PORT, (x))
#define LED_C1_ON(x)            GPIO_ResetBits(LED_C1_PORT, (x))

/* Private variables ---------------------------------------------------------*/
/* The queue used to hold received characters. */
QueueHandle_t xCompleteQueue;
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xReciever1Semaphore = NULL;
SemaphoreHandle_t xReciever2Semaphore = NULL;
SemaphoreHandle_t xReciever3Semaphore = NULL;
SemaphoreHandle_t xReciever4Semaphore = NULL;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION( vMainTask, pvParameters ) {
  int status = INIT;
  char task_flag= 0;
  char temp_flag;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        task_flag= 0;
        LOCK_OFF();
        while (xQueueReceive(xCompleteQueue, &temp_flag, 0));
        status = IDLE;
        break;
      }
      case IDLE: {
        if (xQueueReceive(xCompleteQueue, &temp_flag, portMAX_DELAY)) {
          task_flag |= temp_flag;
          if (0x0F == task_flag)
            status = ACTION;
        }
        break;
      }
      case ACTION: {
        /* Play audio */
        player_play_file(COMPLETE_AUDIO, 0);
        LOCK_ON();
        status = FINISH;
        break;
      }
      case FINISH: {
        /* Reset logic */
        if (pdTRUE == xSemaphoreTake(xResetSemaphore, portMAX_DELAY)) {
          status = INIT;
        }
        break;
      }
    }
  }
}

static portTASK_FUNCTION( vSoldierOneTask, pvParameters ) {
  int status = INIT;
  char task_flag= 0x01;
  int i;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        status = IDLE;
        LED_S1_ON(LED_S1_PIN_ALL);
        DISABLE_RECIEVER_IT(RECIEVER1_PIN);
        while (pdTRUE == xSemaphoreTake(xReciever1Semaphore, 0));
        ENABLE_RECIEVER_IT(RECIEVER1_PIN);
        break;
      }
      case IDLE: {
        for (i = 0; i < 4; i++) {
          if (pdTRUE == xSemaphoreTake(xReciever1Semaphore, DESINTERVAL)) {
            if (3 == i) {
              /* Shooting when the last led is on */
              status = ACTION;
              /* Play audio */
              player_play_file(CORRECT_AUDIO, 0);
            } else {
              status = ERRORLED;
              /* Play audio */
              player_play_file(ERROR_AUDIO, 0);
            }
          }
          LED_S1_OFF(LED_S1_4_PIN >> i);
          if (IDLE != status)
            break;
        }
        if (IDLE == status) {
          DISABLE_RECIEVER_IT(RECIEVER1_PIN);
          vTaskDelay((TickType_t)DESINTERVAL);
          status = ERRORLED;
        }
        break;
      }
      case ERRORLED: {
        for (i = 0; i < (FLASHTIME/600); i++) {
          LED_S1_ON(LED_S1_PIN_ALL);
          vTaskDelay((TickType_t)300);
          LED_S1_OFF(LED_S1_PIN_ALL);
          vTaskDelay((TickType_t)300);
        }
        status = INIT;
        break;
      }
      case ACTION: {
        xQueueSend(xCompleteQueue, &task_flag, 0);
        status = FINISH;
        break;
      }
      case FINISH: {
        /* Reset logic */
        if (pdTRUE == xSemaphoreTake(xResetSemaphore, portMAX_DELAY)) {
          status = INIT;
        }
        break;
      }
    }
  }
} 

static portTASK_FUNCTION( vSoldierTwoTask, pvParameters ) {
  int status = INIT;
  char task_flag= 0x02;
  int i;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        status = IDLE;
        LED_S2_ON(LED_S2_PIN_ALL);
        DISABLE_RECIEVER_IT(RECIEVER2_PIN);
        while (pdTRUE == xSemaphoreTake(xReciever2Semaphore, 0));
        ENABLE_RECIEVER_IT(RECIEVER2_PIN);
        break;
      }
      case IDLE: {
        for (i = 0; i < 4; i++) {
          if (pdTRUE == xSemaphoreTake(xReciever2Semaphore, DESINTERVAL)) {
            if (3 == i) {
              /* Shooting when the last led is on */
              status = ACTION;
              /* Play audio */
              player_play_file(CORRECT_AUDIO, 0);
            } else {
              status = ERRORLED;
              /* Play audio */
              player_play_file(ERROR_AUDIO, 0);
            }
          }
          LED_S2_OFF(LED_S2_4_PIN >> i);
          if (IDLE != status)
            break;
        }
        if (IDLE == status) {
          DISABLE_RECIEVER_IT(RECIEVER2_PIN);
          vTaskDelay((TickType_t)DESINTERVAL);
          status = ERRORLED;
        }
        break;
      }
      case ERRORLED: {
        for (i = 0; i < (FLASHTIME/600); i++) {
          LED_S2_ON(LED_S2_PIN_ALL);
          vTaskDelay((TickType_t)300);
          LED_S2_OFF(LED_S2_PIN_ALL);
          vTaskDelay((TickType_t)300);
        }
        status = INIT;
        break;
      }
      case ACTION: {
        xQueueSend(xCompleteQueue, &task_flag, 0);
        status = FINISH;
        break;
      }
      case FINISH: {
        /* Reset logic */
        if (pdTRUE == xSemaphoreTake(xResetSemaphore, portMAX_DELAY)) {
          status = INIT;
        }
        break;
      }
    }
  }
} 

static portTASK_FUNCTION( vSoldierThreeTask, pvParameters ) {
  int status = INIT;
  char task_flag= 0x04;
  int i;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        status = IDLE;
        LED_S3_ON(LED_S3_PIN_ALL);
        DISABLE_RECIEVER_IT(RECIEVER3_PIN);
        while (pdTRUE == xSemaphoreTake(xReciever3Semaphore, 0));
        ENABLE_RECIEVER_IT(RECIEVER3_PIN);
        break;
      }
      case IDLE: {
        for (i = 0; i < 4; i++) {
          if (pdTRUE == xSemaphoreTake(xReciever3Semaphore, DESINTERVAL)) {
            if (3 == i) {
              /* Shooting when the last led is on */
              status = ACTION;
              /* Play audio */
              player_play_file(CORRECT_AUDIO, 0);
            } else {
              status = ERRORLED;
              /* Play audio */
              player_play_file(ERROR_AUDIO, 0);
            }
          }
          LED_S3_OFF(LED_S3_4_PIN >> i);
          if (IDLE != status)
            break;
        }
        if (IDLE == status) {
          DISABLE_RECIEVER_IT(RECIEVER3_PIN);
          vTaskDelay((TickType_t)DESINTERVAL);
          status = ERRORLED;
        }
        break;
      }
      case ERRORLED: {
        for (i = 0; i < (FLASHTIME/600); i++) {
          LED_S3_ON(LED_S3_PIN_ALL);
          vTaskDelay((TickType_t)300);
          LED_S3_OFF(LED_S3_PIN_ALL);
          vTaskDelay((TickType_t)300);
        }
        status = INIT;
        break;
      }
      case ACTION: {
        xQueueSend(xCompleteQueue, &task_flag, 0);
        status = FINISH;
        break;
      }
      case FINISH: {
        /* Reset logic */
        if (pdTRUE == xSemaphoreTake(xResetSemaphore, portMAX_DELAY)) {
          status = INIT;
        }
        break;
      }
    }
  }
} 

static portTASK_FUNCTION( vCannonTask, pvParameters ) {
  int status = INIT;
  char task_flag= 0x08;
  int i;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        status = IDLE;
        LED_C1_ON(LED_C1_PIN_ALL);
        DISABLE_RECIEVER_IT(RECIEVER4_PIN);
        while (pdTRUE == xSemaphoreTake(xReciever4Semaphore, 0));
        ENABLE_RECIEVER_IT(RECIEVER4_PIN);
        break;
      }
      case IDLE: {
        for (i = 0; i < 6; i++) {
          if (pdTRUE == xSemaphoreTake(xReciever4Semaphore, DESINTERVAL)) {
            if (5 == i) {
              /* Shooting when the last led is on */
              status = ACTION;
              /* Play audio */
              player_play_file(CORRECT_AUDIO, 0);
            } else {
              status = ERRORLED;
              /* Play audio */
              player_play_file(ERROR_AUDIO, 0);
            }
          }
          LED_C1_OFF(LED_C1_6_PIN >> i);
          if (IDLE != status)
            break;
        }
        if (IDLE == status) {
          DISABLE_RECIEVER_IT(RECIEVER4_PIN);
          vTaskDelay((TickType_t)DESINTERVAL);
          status = ERRORLED;
        }
        break;
      }
      case ERRORLED: {
        for (i = 0; i < (FLASHTIME/600); i++) {
          LED_C1_ON(LED_C1_PIN_ALL);
          vTaskDelay((TickType_t)300);
          LED_C1_OFF(LED_C1_PIN_ALL);
          vTaskDelay((TickType_t)300);
        }
        status = INIT;
        break;
      }
      case ACTION: {
        xQueueSend(xCompleteQueue, &task_flag, 0);
        status = FINISH;
        break;
      }
      case FINISH: {
        /* Reset logic */
        if (pdTRUE == xSemaphoreTake(xResetSemaphore, portMAX_DELAY)) {
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
  
  /* Configure IR reciever GPIOs */
  GPIO_InitStructure.GPIO_Pin = RECIEVERS_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(RECIEVERS_PORT, &GPIO_InitStructure);
  
  /* Configure lock GPIO */
  GPIO_SetBits(LOCK_PORT, LOCK_PIN);
  GPIO_InitStructure.GPIO_Pin = LOCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(LOCK_PORT, &GPIO_InitStructure);

  /* Configure LEDS GPIO */
  GPIO_SetBits(LED_S1_PORT, LED_S1_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_S1_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_S1_PORT, &GPIO_InitStructure);

  GPIO_SetBits(LED_S2_PORT, LED_S2_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_S2_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_S2_PORT, &GPIO_InitStructure);

  GPIO_SetBits(LED_S3_PORT, LED_S3_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_S3_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_S3_PORT, &GPIO_InitStructure);

  GPIO_SetBits(LED_C1_PORT, LED_C1_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_C1_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_C1_PORT, &GPIO_InitStructure);
  
  /* Configure recievers interrupts */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);
  
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_InitStructure.EXTI_Line = EXTI_Line1;
  EXTI_Init(&EXTI_InitStructure);
  EXTI_InitStructure.EXTI_Line = EXTI_Line2;
  EXTI_Init(&EXTI_InitStructure);
  EXTI_InitStructure.EXTI_Line = EXTI_Line3;
  EXTI_Init(&EXTI_InitStructure);
  EXTI_InitStructure.EXTI_Line = EXTI_Line4;
  EXTI_Init(&EXTI_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                                    configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
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
}

void init_tasks(void) {
  hardware_init();
  
  /* Create reset semaphore */
  xResetSemaphore = xSemaphoreCreateBinary();  
  
  /* Create switch queue */
  xCompleteQueue = xQueueCreate(4, (unsigned portBASE_TYPE)sizeof(char));
  
  xReciever1Semaphore = xSemaphoreCreateBinary();
  xReciever2Semaphore = xSemaphoreCreateBinary();
  xReciever3Semaphore = xSemaphoreCreateBinary();
  xReciever4Semaphore = xSemaphoreCreateBinary();
  
  xTaskCreate( vMainTask, 
              "Mian", 
              TASKS_STACK_SIZE, 
              NULL, 
              TASKS_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  xTaskCreate( vSoldierOneTask, 
              "SoldierOne", 
              TASKS_STACK_SIZE, 
              NULL, 
              TASKS_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  xTaskCreate( vSoldierTwoTask, 
              "SoldierTwo", 
              TASKS_STACK_SIZE, 
              NULL, 
              TASKS_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  xTaskCreate( vSoldierThreeTask, 
              "SoldierThree", 
              TASKS_STACK_SIZE, 
              NULL, 
              TASKS_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  xTaskCreate( vCannonTask, 
              "Cannon", 
              TASKS_STACK_SIZE, 
              NULL, 
              TASKS_PRIORITY, 
              ( TaskHandle_t * ) NULL );
}

