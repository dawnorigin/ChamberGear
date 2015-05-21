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
#include "SerialIO.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
enum {
  INIT,
  IDLE,
  LED_FLASH,
  PLAY,
  ACTION,
  FINISH
};
/* Private macro -------------------------------------------------------------*/
#define PENTACLE_STACK_SIZE       (configMINIMAL_STACK_SIZE * 2)
#define PENTACLE_PRIORITY			    (tskIDLE_PRIORITY + 1)

#define INPUT_GEAR_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define INPUT_GEAR_PRIORITY		    (tskIDLE_PRIORITY + 1)

#define INPUT_GEAR_PORT           (GPIOA)
#define INPUT_GEAR_PIN            (GPIO_Pin_0)
#define INPUT_GEAR_STATUS()       (GPIO_ReadInputDataBit(INPUT_GEAR_PORT, \
                                                     INPUT_GEAR_PIN))
#define ENABLE_INPUT_GEAR_IT()    {EXTI->IMR |= INPUT_GEAR_PIN;}
#define DISABLE_INPUT_GEAR_IT()   {EXTI->IMR |= INPUT_GEAR_PIN;}

#define ELEMENTS_PORT             (GPIOA)
#define ELEMENT1_PIN              (GPIO_Pin_1)
#define ELEMENT2_PIN              (GPIO_Pin_2)
#define ELEMENT3_PIN              (GPIO_Pin_3)
#define ELEMENT4_PIN              (GPIO_Pin_4)
#define ELEMENT5_PIN              (GPIO_Pin_5)
#define ELEMENTS_PINS             (ELEMENT1_PIN|ELEMENT2_PIN|ELEMENT3_PIN \
                                            |ELEMENT4_PIN|ELEMENT5_PIN)
#define ELEMENTS_STATUS()         GPIO_ReadInputData(ELEMENTS_PORT)

#define PLAYER_BUSY_PORT          (GPIOA)
#define PLAYER_BUSY_PIN           (GPIO_Pin_8)
#define PLAYER_BUSY_STATUS()      (GPIO_ReadInputDataBit(PLAYER_BUSY_PORT, \
                                                     PLAYER_BUSY_PIN))

#define DOOR_PORT                 (GPIOB)
#define DOOR_PIN                  (GPIO_Pin_7)
#define DOOR_OFF()                {GPIO_SetBits(DOOR_PORT, DOOR_PIN);}
#define DOOR_ON()                 {GPIO_ResetBits(DOOR_PORT, DOOR_PIN);}

#define LEDS_PORT                 (GPIOB)
#define LED1_PIN                  (GPIO_Pin_0)
#define LED2_PIN                  (GPIO_Pin_1)
#define LED3_PIN                  (GPIO_Pin_2)
#define LED4_PIN                  (GPIO_Pin_3)
#define LED5_PIN                  (GPIO_Pin_4)
#define LEDS_PINS                 (LED1_PIN|LED2_PIN|LED3_PIN|LED4_PIN|LED5_PIN)
#define LED_OFF(x)                {GPIO_ResetBits(LEDS_PORT, (x));}
#define LED_ON(x)                 {GPIO_SetBits(LEDS_PORT, (x));}

#define LOCK_PORT                 (GPIOB)
#define LOCK_PIN                  (GPIO_Pin_8)
#define LOCK_OFF()                {GPIO_SetBits(LOCK_PORT, LOCK_PIN);}
#define LOCK_ON()                 {GPIO_ResetBits(LOCK_PORT, LOCK_PIN);}

/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xInputSemaphore = NULL;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION( vPentacleTask, pvParameters ) {
  int status = INIT;
  int i;

	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        /* Turn off the lamps */
        DOOR_OFF();
        LED_OFF(LEDS_PINS);
        status = IDLE;
        break;
      }
      case IDLE: {
        LED_ON((ELEMENTS_STATUS() & ELEMENTS_PINS)>>1);
        LED_OFF(((~ELEMENTS_STATUS()) & ELEMENTS_PINS)>>1);
        if (ELEMENT_ACTIVE == (ELEMENTS_STATUS() & ELEMENTS_PINS)) {
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          if (ELEMENT_ACTIVE == (ELEMENTS_STATUS() & ELEMENTS_PINS)) {
            status = LED_FLASH;
          }
        }
        break;
      }
      case LED_FLASH: {
        for (i = 0; i < FLASH_TIMES; i++) {
          LED_ON(LEDS_PINS);
          vTaskDelay((TickType_t)LED_FLASH_DELAY_MS);
          LED_OFF(LEDS_PINS);
          vTaskDelay((TickType_t)LED_FLASH_DELAY_MS);
        }
        status = PLAY;
        break;
      }
      case PLAY: {
        /* Play music */
        player_play_file(FILE_1, 0);
        while(Bit_RESET != PLAYER_BUSY_STATUS());
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        status = ACTION;
        break;
      }
      case ACTION: {
        /* Turn on the lamps */
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

static portTASK_FUNCTION( vInputGearTask, pvParameters ) {
  int status = INIT;

	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        /* Lock the box */
        LOCK_OFF();
        /* Disable play button interrupt */
        DISABLE_INPUT_GEAR_IT();
        while (pdTRUE == xSemaphoreTake(xInputSemaphore, 0));
        /* Enable play button interrupt */
        ENABLE_INPUT_GEAR_IT();
        status = IDLE;
        break;
      }
      case IDLE: {
        /* Occur when play button is pressed */        
        if (pdTRUE == xSemaphoreTake(xInputSemaphore, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          /* Check whether the button was pressed */
          if (Bit_RESET == INPUT_GEAR_STATUS()) {
            status = ACTION;
          } else {
            /* Enable play button interrupt */
            ENABLE_INPUT_GEAR_IT();
          }
        }
        break;
      }
      case ACTION: {
        /* Turn on the lamps */
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
  GPIO_InitStructure.GPIO_Pin = ELEMENTS_PINS;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(ELEMENTS_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = LEDS_PINS;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LEDS_PORT, &GPIO_InitStructure);
  
  GPIO_SetBits(DOOR_PORT, DOOR_PIN);
  GPIO_InitStructure.GPIO_Pin = DOOR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DOOR_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PLAYER_BUSY_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PLAYER_BUSY_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = INPUT_GEAR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(INPUT_GEAR_PORT, &GPIO_InitStructure);

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
//  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);
//  
//	EXTI_InitStructure.EXTI_Line = EXTI_Line6;
//  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
//  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//  EXTI_Init(&EXTI_InitStructure);  
//  
//  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
//                          configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
  
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
  
  xTaskCreate( vPentacleTask, 
              "Pentacle", 
              PENTACLE_STACK_SIZE, 
              NULL, 
              PENTACLE_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  xInputSemaphore = xSemaphoreCreateBinary();
  xTaskCreate( vInputGearTask, 
              "InputGear", 
              INPUT_GEAR_STACK_SIZE, 
              NULL, 
              INPUT_GEAR_PRIORITY, 
              ( TaskHandle_t * ) NULL );
}

