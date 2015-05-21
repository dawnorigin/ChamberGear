/* Includes ------------------------------------------------------------------*/
/* Library includes. */
#include "stm32f10x.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
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
#define PAINTING_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)
#define PAINTING_PRIORITY   (tskIDLE_PRIORITY + 1)

#define WEIGHT_STACK_SIZE   (configMINIMAL_STACK_SIZE * 3)
#define WEIGHT_PRIORITY     (tskIDLE_PRIORITY + 1)

#define DOOR_PORT           (GPIOB)
#define DOOR_PIN            (GPIO_Pin_0)
#define DOOR_OFF()          {GPIO_SetBits(DOOR_PORT, DOOR_PIN);}
#define DOOR_ON()           {GPIO_ResetBits(DOOR_PORT, DOOR_PIN);}

#define LOCK_PORT           (GPIOB)
#define LOCK_PIN            (GPIO_Pin_1)
#define LOCK_OFF()          {GPIO_SetBits(LOCK_PORT, LOCK_PIN);}
#define LOCK_ON()           {GPIO_ResetBits(LOCK_PORT, LOCK_PIN);}

#define SWITCH_PORT         (GPIOA)
#define SWITCH_STATUS(x)    GPIO_ReadInputDataBit(SWITCH_PORT, (1 << (x)))
#define ENABLE_SWITCH_IT(x) {EXTI->IMR |= (1 << (x));}

#define AD_DOUT_PORT        (GPIOA)
#define AD_DOUT_PIN         (GPIO_Pin_0)
#define AD_DOUT_STATUS()    GPIO_ReadInputDataBit(AD_DOUT_PORT, AD_DOUT_PIN)

#define AD_CLK_PORT         (GPIOB)
#define AD_CLK_PIN          (GPIO_Pin_15)
#define AD_CLK_SET()        GPIO_SetBits(AD_CLK_PORT, AD_CLK_PIN)
#define AD_CLK_RESET()      GPIO_ResetBits(AD_CLK_PORT, AD_CLK_PIN)
/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
/* The queue used to hold received characters. */
QueueHandle_t xSwitchQueue;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION( vPaintingTask, pvParameters ) {
  int status = INIT;
  char switch_no;
  char switch_passed;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        switch_passed = 0;
        status = IDLE;
        LOCK_OFF();
        while (xQueueReceive(xSwitchQueue, &switch_no, 0));
        ENABLE_SWITCH_IT(1);
        break;
      }
      case IDLE: {
        /* Wait for any switch event */  
        if (pdTRUE == xQueueReceive(xSwitchQueue, &switch_no, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          /* Check whether the switch is passing*/
          if (Bit_RESET == SWITCH_STATUS(switch_no)) {
            if ((switch_passed + 1) == switch_no) {
              /* The passsing switch is active */
              switch_passed++;
              if (SWITCH_NUMBER == switch_passed) {
                /* The last switch is passing */
                status = ACTION;
              } else {
                ENABLE_SWITCH_IT(switch_no);
              }
            } else {
              /* The passing switch is inactive */
              status = INIT;
              ENABLE_SWITCH_IT(switch_no);
            }
          } else {
            /* Enable play button interrupt */
            ENABLE_SWITCH_IT(switch_no);
          }
        }
        break;
      }
      case ACTION: {
        /* Open the box */
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


int get_weight_raw(void) {
  int weight = 0,i;
  
  /* Wait for HX711 ready */
  while(Bit_RESET != AD_DOUT_STATUS());
  /* Crttical segment to make any postive plus time be less than 50us */
  taskENTER_CRITICAL();
  for (i = 0; i< 24; i++) {
    AD_CLK_SET();
    weight <<= 1;
    AD_CLK_RESET();
    if (Bit_RESET != AD_DOUT_STATUS())
      weight++;
  }
  AD_CLK_SET();
  weight ^= 0x800000;
  AD_CLK_RESET();
  taskEXIT_CRITICAL();
  
  return (weight);
}


static portTASK_FUNCTION( vWeightTask, pvParameters ) {
  int status = INIT;
  int weight;
    
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        status = IDLE;
        DOOR_OFF();
        break;
      }
      case IDLE: {
        vTaskDelay((TickType_t)WEIGHT_TEST_INTERVAL);
        weight = get_weight_raw();
        if (weight > WEIGHT_THRESHOLD)
          status = ACTION;
        break;
      }
      case ACTION: {
        /* Open the door */
        DOOR_ON();
        /* Play music */
        player_play_file(FILE_1, 0);
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
  int i;
  
  /* Close the door */
  GPIO_SetBits(DOOR_PORT, DOOR_PIN);
  GPIO_InitStructure.GPIO_Pin = DOOR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DOOR_PORT, &GPIO_InitStructure);
  
  /* Close the lock */
  GPIO_SetBits(LOCK_PORT, LOCK_PIN);
  GPIO_InitStructure.GPIO_Pin = LOCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LOCK_PORT, &GPIO_InitStructure);
  
  /* Configure HX711 pins */
  AD_CLK_RESET();
  GPIO_InitStructure.GPIO_Pin = AD_CLK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(AD_CLK_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = AD_DOUT_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(AD_DOUT_PORT, &GPIO_InitStructure);
  
  /* Configure switch GPIOs and interrupts */
  for (i = 1; i <= SWITCH_NUMBER; i++)
  {
    GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_0 << i);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWITCH_PORT, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, (GPIO_PinSource0 << i));
    
    EXTI_InitStructure.EXTI_Line = (EXTI_Line0 << i);
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
  }

  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                                    configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  switch (SWITCH_NUMBER) {
    case 15:
    case 14:
    case 13:
    case 12:
    case 11:
    case 10:
      NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
      NVIC_Init(&NVIC_InitStructure);
    case 9:
    case 8:
    case 7:
    case 6:
    case 5:
      NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
      NVIC_Init(&NVIC_InitStructure);
    case 4:
      NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
      NVIC_Init(&NVIC_InitStructure);
    case 3:
      NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
      NVIC_Init(&NVIC_InitStructure);
    case 2:
      NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
      NVIC_Init(&NVIC_InitStructure);
    case 1:
      NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
      NVIC_Init(&NVIC_InitStructure);
  }
  
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
  hardware_init();
  
  /* Create reset semaphore */
  xResetSemaphore = xSemaphoreCreateBinary();
  
  /* Create switch queue */
  xSwitchQueue = xQueueCreate( SWITCH_NUMBER, 
                          ( unsigned portBASE_TYPE ) sizeof( signed char ) );
  /* Create painting task */
  xTaskCreate( vPaintingTask, 
              "Painting", 
              PAINTING_STACK_SIZE, 
              NULL, 
              PAINTING_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  /* Create weight task */
  xTaskCreate( vWeightTask, 
              "Weight", 
              WEIGHT_STACK_SIZE, 
              NULL, 
              WEIGHT_PRIORITY, 
              ( TaskHandle_t * ) NULL );
}

