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
#define PLAYBOTTON_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define PLAYBOTTON_PRIORITY			  (tskIDLE_PRIORITY + 1)

#define HITBOTTON_STACK_SIZE      (configMINIMAL_STACK_SIZE * 3)
#define HITBOTTON_PRIORITY			  (tskIDLE_PRIORITY + 1)

#define PLAY_BUTTON_PORT          (GPIOA)
#define PLAY_BUTTON_PIN           (GPIO_Pin_0)
#define PLAY_BUTTON_STATUS()      (GPIO_ReadInputDataBit(PLAY_BUTTON_PORT, \
                                                     PLAY_BUTTON_PIN))
#define ENABLE_PLAY_BUTTON_IT()   {EXTI->IMR |= EXTI_Line0;}
#define DISABLE_PLAY_BUTTON_IT()  {EXTI->IMR &= ~EXTI_Line0;}

#define HIT_BUTTON_PORT           (GPIOA)
#define HIT_BUTTON_PIN            (GPIO_Pin_2)
#define HIT_BUTTON_STATUS()       (GPIO_ReadInputDataBit(HIT_BUTTON_PORT, \
                                                     HIT_BUTTON_PIN))
#define ENABLE_HIT_BUTTON_IT()    {EXTI->IMR |= EXTI_Line2;}
#define DISABLE_HIT_BUTTON_IT()   {EXTI->IMR &= ~EXTI_Line2;}

#define LAMP_PORT                 (GPIOB)
#define LAMP_PIN                  (GPIO_Pin_0)
#define LAMP_OFF()                {GPIO_SetBits(LAMP_PORT, LAMP_PIN);}
#define LAMP_ON()                 {GPIO_ResetBits(LAMP_PORT, LAMP_PIN);}

#define LOCK_PORT                 (GPIOB)
#define LOCK_PIN                  (GPIO_Pin_1)
#define LOCK_OFF()                {GPIO_SetBits(LOCK_PORT, LOCK_PIN);}
#define LOCK_ON()                 {GPIO_ResetBits(LOCK_PORT, LOCK_PIN);}

/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xBottonSemaphore = NULL;
SemaphoreHandle_t xHitSemaphore = NULL;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION( vPlayBottonTask, pvParameters ) {
  int status = INIT;

	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        /* Turn off the lamps */
        LAMP_OFF();
        /* Disable play button interrupt */
        DISABLE_PLAY_BUTTON_IT();
        /* Clean play button semaphore */
        while (pdTRUE == xSemaphoreTake(xBottonSemaphore, 0));
        while (Bit_RESET == PLAY_BUTTON_STATUS()) {
          vTaskDelay((TickType_t)10);
        }
        vTaskDelay((TickType_t)START_DELAY_MS);
        /* Enable play button interrupt */
        ENABLE_PLAY_BUTTON_IT();
        status = IDLE;
        break;
      }
      case IDLE: {
        /* Occur when play button is pressed */        
        if (pdTRUE == xSemaphoreTake(xBottonSemaphore, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          /* Check whether the button was pressed */
          if (Bit_RESET == PLAY_BUTTON_STATUS()) {
            status = ACTION;
          } else {/* Enable play button interrupt */
            ENABLE_PLAY_BUTTON_IT();
          }
        }
        break;
      }
      case ACTION: {
        /* Turn on the lamps */
        LAMP_ON();
        /* Play audio */
        player_play_file(PLAY_BUTTON_AUDIO, 0);
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

static portTASK_FUNCTION( vHitTask, pvParameters ) {
  int status = INIT;
  int hit_counter = 0;  
  unsigned short hit_interval[HIT_BUTTON_TIMES - 1];
  int hit_interval_index = 0;
  TickType_t tmp_tick, hit_period, hit_previous_tick = 0;  
  int i;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        /* Lock up the box */
        LOCK_OFF();
        /* Empty hit interval buffer */
        for (i = 0; i< (HIT_BUTTON_TIMES - 1); i++)
          hit_interval[i] = 0;
        hit_period = 0;
        hit_counter = 0;
        hit_interval_index = 0;
        /* Disable play button interrupt */
        DISABLE_HIT_BUTTON_IT();
        /* Clean play button semaphore */
        while (pdTRUE == xSemaphoreTake(xHitSemaphore, 0));
        /* Enable hit button interrupt */
        ENABLE_HIT_BUTTON_IT();
        status = IDLE;
        break;
      }
      case IDLE: {
        /* Occur when play button is pressed */        
        if (pdTRUE == xSemaphoreTake(xHitSemaphore, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          /* Check whether the button was pressed */
          if (Bit_SET == HIT_BUTTON_STATUS()) {
            hit_previous_tick = xTaskGetTickCount();
            hit_counter++;
            status = PROCESS;
          }
          /* Enable hit button interrupt */
          ENABLE_HIT_BUTTON_IT();
        }
        
        break;
      }
      case PROCESS: {
        /* Occur when play button is pressed */        
        if (pdTRUE == xSemaphoreTake(xHitSemaphore, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          /* Check whether the button was pressed */
          if (Bit_SET == HIT_BUTTON_STATUS()) {
            /* Update hit interval */
            tmp_tick = xTaskGetTickCount();
            hit_period -= hit_interval[hit_interval_index];
            hit_interval[hit_interval_index] = tmp_tick - hit_previous_tick;
            hit_period += hit_interval[hit_interval_index];
            hit_previous_tick = tmp_tick;
            hit_interval_index++;
            if (hit_interval_index == HIT_BUTTON_TIMES - 1)
              hit_interval_index = 0;
            /* Increase hit counter */
            hit_counter++;
            
            if(hit_counter >= HIT_BUTTON_TIMES) {
              hit_counter = HIT_BUTTON_TIMES;
              if (hit_period <= HIT_BUTTON_PERIOD) {
                status = ACTION;
              } else {
                ENABLE_HIT_BUTTON_IT();
              }
            } else {
              ENABLE_HIT_BUTTON_IT();
            }
          } else {
            ENABLE_HIT_BUTTON_IT();
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

static void hardware_init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  
  /* Configure GPIO */
  GPIO_InitStructure.GPIO_Pin = PLAY_BUTTON_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = HIT_BUTTON_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_SetBits(LAMP_PORT, LAMP_PIN);
  GPIO_InitStructure.GPIO_Pin = LAMP_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(LAMP_PORT, &GPIO_InitStructure);
  
  GPIO_SetBits(LOCK_PORT, LOCK_PIN);
  GPIO_InitStructure.GPIO_Pin = LOCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(LOCK_PORT, &GPIO_InitStructure);
  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                          configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                          configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);  
  
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);  

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
  
  xBottonSemaphore = xSemaphoreCreateBinary();
  xTaskCreate( vPlayBottonTask, 
              "PlayBotton", 
              PLAYBOTTON_STACK_SIZE, 
              NULL, 
              PLAYBOTTON_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  xHitSemaphore = xSemaphoreCreateBinary();
  xTaskCreate( vHitTask, 
              "HitBotton", 
              HITBOTTON_STACK_SIZE, 
              NULL, 
              HITBOTTON_PRIORITY, 
              ( TaskHandle_t * ) NULL );
}

