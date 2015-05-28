/* Includes ------------------------------------------------------------------*/
/* Library includes. */
#include "stm32f10x.h"
#include <stdlib.h>
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
  IDLE_BLUE,
  IDLE_GREEN,
  IDLE_PINK,
  IDLE_YELLOW,
  DJPOWERON,
  DJ_BLUE,
  DJ_GREEN,
  DJ_PINK,
  BUTTON_ERR,
  TREAD_ERR,
  ACTION,
  FINISH
};
/* Private macro -------------------------------------------------------------*/
#define TREAD_STACK_SIZE        (configMINIMAL_STACK_SIZE)
#define TREAD_PRIORITY			    (tskIDLE_PRIORITY + 2)

#define ENTRANCE_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define ENTRANCE_PRIORITY			  (tskIDLE_PRIORITY + 1)

#define CRAB_STACK_SIZE         (configMINIMAL_STACK_SIZE * 2)
#define CRAB_PRIORITY			      (tskIDLE_PRIORITY + 1)

#define SETS_STACK_SIZE         (configMINIMAL_STACK_SIZE * 2)
#define SETS_PRIORITY			      (tskIDLE_PRIORITY + 1)

#define DJ_STACK_SIZE           (configMINIMAL_STACK_SIZE * 2)
#define DJ_PRIORITY			        (tskIDLE_PRIORITY + 1)

#define PYRO_PORT           (GPIOA)
#define PYRO_PIN            (GPIO_Pin_0)
#define PYRO_STATUS()       GPIO_ReadInputDataBit(PYRO_PORT, PYRO_PIN)
#define ENABLE_PYRO_IT()    {EXTI->IMR |= PYRO_PIN;}
#define DISABLE_PYRO_IT()   {EXTI->IMR &= (~PYRO_PIN);}

#define CRAB_PORT           (GPIOA)
#define CRAB_PIN            (GPIO_Pin_1)
#define CRAB_STATUS()       GPIO_ReadInputDataBit(CRAB_PORT, CRAB_PIN)
#define ENABLE_CRAB_IT()    {EXTI->IMR |= CRAB_PIN;}
#define DISABLE_CRAB_IT()   {EXTI->IMR &= (~CRAB_PIN);}

#define SET1_PORT           (GPIOA)
#define SET1_PIN            (GPIO_Pin_2)
#define SET1_STATUS()       GPIO_ReadInputDataBit(SET1_PORT, SET1_PIN)
#define ENABLE_SET1_IT()    {EXTI->IMR |= SET1_PIN;}
#define DISABLE_SET1_IT()   {EXTI->IMR &= (~SET1_PIN);}

#define SET2_PORT           (GPIOA)
#define SET2_PIN            (GPIO_Pin_3)
#define SET2_STATUS()       GPIO_ReadInputDataBit(SET2_PORT, SET2_PIN)
#define ENABLE_SET2_IT()    {EXTI->IMR |= SET2_PIN;}
#define DISABLE_SET2_IT()   {EXTI->IMR &= (~SET2_PIN);}

#define PLAYER_BUSY_PORT      (GPIOC)
#define PLAYER_BUSY_PIN       (GPIO_Pin_14)
#define PLAYER_BUSY_STATUS()  (GPIO_ReadInputDataBit(PLAYER_BUSY_PORT, \
                                                     PLAYER_BUSY_PIN))

#define TREAD_PORT          (GPIOA)
#define TREAD1_PIN          (GPIO_Pin_5)
#define TREAD2_PIN          (GPIO_Pin_6)
#define TREAD3_PIN          (GPIO_Pin_8)
#define TREAD4_PIN          (GPIO_Pin_9)
#define TREAD_PIN_ALL       (TREAD1_PIN|TREAD2_PIN|TREAD3_PIN|TREAD4_PIN)
#define TREAD_STATUS(x)     GPIO_ReadInputDataBit(TREAD_PORT, (x))
#define ENABLE_TREAD_IT(x)  {EXTI->IMR |= (x);}
#define DISABLE_TREAD_IT(x) {EXTI->IMR &= (~(x));}

#define BUTTON_PORT           (GPIOA)
#define BUTTON1_PIN           (GPIO_Pin_10)
#define BUTTON2_PIN           (GPIO_Pin_11)
#define BUTTON3_PIN           (GPIO_Pin_12)
#define BUTTON4_PIN           (GPIO_Pin_15)
#define BUTTON_PIN_ALL        (BUTTON1_PIN|BUTTON2_PIN|BUTTON3_PIN|BUTTON4_PIN)
#define BUTTON_STATUS(x)      GPIO_ReadInputDataBit(BUTTON_PORT, (x))
#define ENABLE_BUTTON_IT(x)   {EXTI->IMR |= (x);}
#define DISABLE_BUTTON_IT(x)  {EXTI->IMR &= (~(x));}

/* Output pins */
#define LAMP_PORT             (GPIOB)
#define LAMP1_PIN             (GPIO_Pin_0)
#define LAMP2_PIN             (GPIO_Pin_1)
#define LAMP3_PIN             (GPIO_Pin_2)
#define LAMP4_PIN             (GPIO_Pin_3)
#define LAMP_PIN_ALL          (LAMP1_PIN|LAMP2_PIN|LAMP3_PIN|LAMP4_PIN)
#define LAMP_OFF(x)           {GPIO_SetBits(LAMP_PORT, (x));}
#define LAMP_ON(x)            {GPIO_ResetBits(LAMP_PORT, (x));}

#define LAMP_BLUE             LAMP1_PIN
#define LAMP_GREEN            LAMP2_PIN
#define LAMP_PINK             LAMP3_PIN
#define LAMP_YELLOW           LAMP4_PIN

#define WALL_PORT             (GPIOB)
#define WALL_PIN              (GPIO_Pin_4)
#define WALL_OFF()            {GPIO_SetBits(WALL_PORT, WALL_PIN);}
#define WALL_ON()             {GPIO_ResetBits(WALL_PORT, WALL_PIN);}

#define WALL_LAMP_PORT        (GPIOA)
#define WALL_LAMP_PIN         (GPIO_Pin_4)
#define WALL_LAMP_OFF()       {GPIO_SetBits(WALL_LAMP_PORT, WALL_LAMP_PIN);}
#define WALL_LAMP_ON()        {GPIO_ResetBits(WALL_LAMP_PORT, WALL_LAMP_PIN);}

#define USART1_PORT           (GPIOB)
#define USART1_TX_PIN         (GPIO_Pin_6)
#define USART1_RX_PIN         (GPIO_Pin_7)

#define CAST_LAMP_PORT        (GPIOB)
#define CAST_LAMP1_PIN        (GPIO_Pin_8)
#define CAST_LAMP2_PIN        (GPIO_Pin_9)
#define CAST_LAMP3_PIN        (GPIO_Pin_10)
#define CAST_LAMP4_PIN        (GPIO_Pin_11)
#define CAST_LAMP_PIN_ALL     (CAST_LAMP1_PIN|CAST_LAMP2_PIN| \
                               CAST_LAMP3_PIN|CAST_LAMP4_PIN)
#define CAST_LAMP_OFF(x)      {GPIO_SetBits(CAST_LAMP_PORT, (x));}
#define CAST_LAMP_ON(x)       {GPIO_ResetBits(CAST_LAMP_PORT, (x));}

#define LED_PORT              (GPIOB)
#define LED1_PIN              (GPIO_Pin_12)
#define LED2_PIN              (GPIO_Pin_13)
#define LED3_PIN              (GPIO_Pin_14)
#define LED4_PIN              (GPIO_Pin_15)
#define LED_PIN_ALL           (LED1_PIN|LED2_PIN|LED3_PIN|LED4_PIN)
#define LED_OFF(x)            {GPIO_SetBits(LED_PORT, (x));}
#define LED_ON(x)             {GPIO_ResetBits(LED_PORT, (x));}

#define DOOR_PORT             (GPIOB)
#define DOOR_PIN              (GPIO_Pin_5)
#define DOOR_OFF()            {GPIO_SetBits(DOOR_PORT, DOOR_PIN);}
#define DOOR_ON()             {GPIO_ResetBits(DOOR_PORT, DOOR_PIN);}

#define LAMP_ERR_PORT         (GPIOA)
#define LAMP_ERR_PIN          (GPIO_Pin_7)
#define LAMP_ERR_OFF()        {GPIO_SetBits(LAMP_ERR_PORT, LAMP_ERR_PIN);}
#define LAMP_ERR_ON()         {GPIO_ResetBits(LAMP_ERR_PORT, LAMP_ERR_PIN);}
/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xPyroSemaphore = NULL;
SemaphoreHandle_t xCrabSemaphore = NULL;
SemaphoreHandle_t xSet1Semaphore = NULL;
SemaphoreHandle_t xSet2Semaphore = NULL;
SemaphoreHandle_t xDJPowerOnSemaphore = NULL;
/* The queue used to hold step trod */
QueueHandle_t xTreadQueue;
SemaphoreHandle_t xTreadSemaphore[4];
/* The queue used to hold button pressed event */
QueueHandle_t xButtonQueue;

uint8_t tasks[4] = {0, 1, 2, 3};
uint16_t treads[4] = {TREAD1_PIN, TREAD2_PIN, TREAD3_PIN, TREAD4_PIN};
uint16_t cast_lamp[4] = {CAST_LAMP1_PIN, CAST_LAMP2_PIN, 
                         CAST_LAMP3_PIN, CAST_LAMP4_PIN};
uint16_t button_led[4] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN};
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION( vEntranceTask, pvParameters ) {
  int status = INIT;
  int i;
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        LAMP_ERR_OFF();
        DISABLE_PYRO_IT();
        while (pdTRUE == xSemaphoreTake(xPyroSemaphore, 0));
        ENABLE_PYRO_IT();
        status = IDLE;
        break;
      }
      case IDLE: {
        if (pdTRUE == xSemaphoreTake(xPyroSemaphore, portMAX_DELAY)) {
          player_play_file(ENTRANCE_AUDIO, 0);
          for (i = 0; i < LAMP_ERROR_FLASH_TIMES; i++) {
            LAMP_ERR_ON();
            vTaskDelay((TickType_t)LAMP_ERROR_FLASH_MS);
            LAMP_ERR_OFF();
            vTaskDelay((TickType_t)LAMP_ERROR_FLASH_MS);
          }
          status = FINISH;
        }
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

static portTASK_FUNCTION( vCrabTask, pvParameters ) {
  int status = INIT;

	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        WALL_OFF();
        WALL_LAMP_OFF();
        DISABLE_CRAB_IT();
        while (pdTRUE == xSemaphoreTake(xCrabSemaphore, 0));
        ENABLE_CRAB_IT();
        status = IDLE;
        break;
      }
      case IDLE: {
        /* Occur when the crab is removed */        
        if (pdTRUE == xSemaphoreTake(xCrabSemaphore, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          /* Check crab hall switch status */
          if (Bit_RESET != CRAB_STATUS()) {
            status = ACTION;
          } else {
            ENABLE_CRAB_IT();
          }
        }
        break;
      }
      case ACTION: {
        player_play_file(CRAB_REMOVE_AUDIO, 0);
        /* Drop the wall */
        WALL_ON();
        /* Turn on the cast light */
        WALL_LAMP_ON();
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

static portTASK_FUNCTION( vSetsTask, pvParameters ) {
  int status = INIT;

	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        DISABLE_SET1_IT();
        DISABLE_SET2_IT()
        status = IDLE;
        break;
      }
      case IDLE: {
        if ((Bit_RESET == SET1_STATUS()) && (Bit_RESET == SET2_STATUS())) {
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          if ((Bit_RESET == SET1_STATUS()) && (Bit_RESET == SET2_STATUS())) {
            status = ACTION;
          }
        }
        vTaskDelay((TickType_t)10);
        break;
      }
      case ACTION: {
        player_play_file(CRAB_SET_AUDIO, 0);
        while (Bit_RESET != PLAYER_BUSY_STATUS());
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        xSemaphoreGive(xDJPowerOnSemaphore);
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
int status = INIT;
static portTASK_FUNCTION( vDJTask, pvParameters ) {
  
  IT_event event;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        LAMP_OFF(LAMP_PIN_ALL);
        LED_OFF(LED_PIN_ALL);
        CAST_LAMP_OFF(CAST_LAMP_PIN_ALL);
        DOOR_OFF();
        /* Clean button semaphore */
        DISABLE_BUTTON_IT(BUTTON_PIN_ALL);
        while (pdTRUE == xQueueReceive(xButtonQueue, &event, 0));
        /* Clean tread semaphore */
        DISABLE_TREAD_IT(TREAD_PIN_ALL);
        while (pdTRUE == xQueueReceive(xTreadQueue, &event, 0));
        /* Clean DJ power on semaphore */
        while (pdTRUE == xSemaphoreTake(xDJPowerOnSemaphore, 0));
        /* Wait for DJ power on semaphore */
        if (pdTRUE == xSemaphoreTake(xDJPowerOnSemaphore, portMAX_DELAY)) {
          status = DJPOWERON;
        }
        break;
      }
      case DJPOWERON: {
        player_play_file(DJ_POWER_ON_AUDIO, 0);
        LAMP_ON(LAMP_PIN_ALL);
        LED_ON(LED_PIN_ALL);
        CAST_LAMP_ON(CAST_LAMP_PIN_ALL);
        vTaskDelay((TickType_t)1000);
        LAMP_OFF(LAMP_PIN_ALL);
        LED_OFF(LED_PIN_ALL);
        CAST_LAMP_OFF(CAST_LAMP_PIN_ALL);
        status = IDLE_BLUE;
        break;
      }
      case IDLE_BLUE:
      case IDLE_GREEN:
      case IDLE_PINK:
      case IDLE_YELLOW: {
        LAMP_OFF(LAMP_PIN_ALL);
        ENABLE_BUTTON_IT(BUTTON_PIN_ALL);
        /* Occur when the crab is removed */        
        if (pdTRUE == xQueueReceive(xButtonQueue, &event, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          event.event &= BUTTON_PIN_ALL;
          if (Bit_RESET == BUTTON_STATUS(event.event)) {
            if ((BUTTON1_PIN == (event.event & BUTTON1_PIN)) 
                && (IDLE_BLUE == status)) {
              status = DJ_BLUE;
              LED_ON(LED1_PIN);
              LAMP_ON(LAMP1_PIN);
              player_play_file(DJ_BLUE_AUDIO, 0);
            } else if ((BUTTON2_PIN == (event.event & BUTTON2_PIN)) 
                       && (IDLE_GREEN == status)) {
              status = DJ_GREEN;
              LED_ON(LED2_PIN);
              LAMP_ON(LAMP2_PIN);
              player_play_file(DJ_GREEN_AUDIO, 0);
            } else if ((BUTTON3_PIN == (event.event & BUTTON3_PIN)) 
                       && (IDLE_PINK == status)) {
              status = DJ_PINK;
              LED_ON(LED3_PIN);
              LAMP_ON(LAMP3_PIN);
              player_play_file(DJ_PINK_AUDIO, 0);
            } else if ((BUTTON4_PIN == (event.event & BUTTON4_PIN)) 
                       && (IDLE_YELLOW == status)) {
              status = ACTION;
              LED_ON(LED4_PIN);
              break;
            } else {
              status = BUTTON_ERR;
              break;
            }
          }
          ENABLE_BUTTON_IT(BUTTON_PIN_ALL);
        }
        break;
      }
      case DJ_BLUE:
      case DJ_GREEN:
      case DJ_PINK: {
        srand(xTaskGetTickCount());
        vTaskDelay((TickType_t)3000);
        
        int index;
        int i = DJ_TREAD_RANDOM_TIMES;
        
        while (0 != i) {
          ENABLE_TREAD_IT(TREAD_PIN_ALL);
          index = (rand() / (RAND_MAX / 4));
          CAST_LAMP_ON(cast_lamp[index]);
          if (pdTRUE == xQueueReceive(xTreadQueue, &event, DJ_TREAD_DELAY_MS)) {
            if (index == event.event) {
              player_play_file(DJ_TREAD_CORRECT_AUDIO, 0);
              CAST_LAMP_OFF(CAST_LAMP_PIN_ALL);
              i--;
              continue;
            } else {
              status = TREAD_ERR;
              break;
            }
          } else { // Timeout
            DISABLE_TREAD_IT(TREAD_PIN_ALL);
            status = TREAD_ERR;
            break;
          }
        }//for(;;)
        CAST_LAMP_OFF(CAST_LAMP_PIN_ALL);
        player_play_file(DJ_TREAD_COMPLETE_AUDIO, 0);
        switch (status) {
          case DJ_BLUE:   status = IDLE_GREEN;break;
          case DJ_GREEN:  status = IDLE_PINK;break;
          case DJ_PINK:   status = IDLE_YELLOW;break;
        }
        break;
      }
      case BUTTON_ERR: {
        player_play_file(DJ_BUTTON_ERR_AUDIO, 0);
        LED_OFF(LED_PIN_ALL);
        LAMP_OFF(LAMP_PIN_ALL);
        LAMP_ERR_ON();
        vTaskDelay((TickType_t)1000);
        LAMP_ERR_OFF();
        status = IDLE_BLUE;
        break;
      }
      case TREAD_ERR: {
        player_play_file(DJ_TREAD_ERR_AUDIO, 0);
        LED_OFF(LED_PIN_ALL);
        LAMP_OFF(LAMP_PIN_ALL);
        LAMP_ERR_ON();
        vTaskDelay((TickType_t)1000);
        LAMP_ERR_OFF();
        status = IDLE_BLUE;
        break;
      }
      case ACTION: {
        player_play_file(DOOR_OPEN_AUDIO, 0);
        while (Bit_RESET != PLAYER_BUSY_STATUS());
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
        if (pdTRUE == xSemaphoreTake(xResetSemaphore, portMAX_DELAY)) {
          status = INIT;
        }
        break;
      }
    }
  }
}

static portTASK_FUNCTION(vTreadTask, pvParameters) {
  int index = *((char*)pvParameters);
  IT_event event;
  
  event.event = index;
  for(;;) {
    if (pdTRUE == xSemaphoreTake(xTreadSemaphore[index], portMAX_DELAY)) {
      /* Skip the key jitter step */
      vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
      if (Bit_RESET == TREAD_STATUS(treads[index])) {
        xQueueSend(xTreadQueue, &event, 0);
      }
      ENABLE_TREAD_IT(treads[index]);
    }
  }//for(;;)
}

static void hardware_init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  
  /* Configure input GPIO */
  GPIO_InitStructure.GPIO_Pin = PYRO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PYRO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = CRAB_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CRAB_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = SET1_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SET1_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = SET2_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SET2_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PLAYER_BUSY_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PLAYER_BUSY_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = TREAD_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(TREAD_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = BUTTON_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BUTTON_PORT, &GPIO_InitStructure);
  
  /* Configure output GPIO */
  LAMP_OFF(LAMP_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LAMP_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LAMP_PORT, &GPIO_InitStructure);
  
  WALL_OFF();
  GPIO_InitStructure.GPIO_Pin = WALL_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(WALL_PORT, &GPIO_InitStructure);

  WALL_LAMP_OFF();
  GPIO_InitStructure.GPIO_Pin = WALL_LAMP_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(WALL_LAMP_PORT, &GPIO_InitStructure);
  
  CAST_LAMP_OFF(CAST_LAMP_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = CAST_LAMP_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CAST_LAMP_PORT, &GPIO_InitStructure);

  LED_OFF(LED_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_PORT, &GPIO_InitStructure);
  
  DOOR_OFF();
  GPIO_InitStructure.GPIO_Pin = DOOR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(DOOR_PORT, &GPIO_InitStructure);
  
  LAMP_ERR_OFF();
  GPIO_InitStructure.GPIO_Pin = LAMP_ERR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(LAMP_ERR_PORT, &GPIO_InitStructure);
  
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource9);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource10);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource11);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource12);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource15);
  
	EXTI_InitStructure.EXTI_Line = PYRO_PIN|CRAB_PIN;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);  
	
	EXTI_InitStructure.EXTI_Line = SET1_PIN|SET2_PIN|TREAD_PIN_ALL|BUTTON_PIN_ALL;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                          configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
  NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable USART clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
  
  /* Configure USART1 Tx (PB.06) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  /* Configure USART1 Rx (PB.07) as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStructure);  
  
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
  char i;
  char task_name[configMAX_TASK_NAME_LEN] = {'T','r','e','a','d','1',0};
  /* Initialize hardware */
  hardware_init();

  /* Create reset semaphore */
  xResetSemaphore = xSemaphoreCreateBinary();

  /* Create pyro detector semaphore */
  xPyroSemaphore = xSemaphoreCreateBinary();

  /* Create crab remove semaphore */
  xCrabSemaphore = xSemaphoreCreateBinary();
  
  /* Create crab set 1 semaphore */
  xSet1Semaphore = xSemaphoreCreateBinary();
  
  /* Create crab set 2 semaphore */
  xSet2Semaphore = xSemaphoreCreateBinary();
  
  /* Create DJ power on semaphore */
  xDJPowerOnSemaphore = xSemaphoreCreateBinary();
    
  /* Create tread event queue */
  xTreadQueue = xQueueCreate(20, (unsigned portBASE_TYPE)sizeof(IT_event));  
  
  /* Create button event queue */
  xButtonQueue = xQueueCreate(20, (unsigned portBASE_TYPE)sizeof(IT_event)); 
  
  for (i = 0; i < 4; i++) {
    xTreadSemaphore[i] = xSemaphoreCreateBinary();
    task_name[5] = i + '1';
    xTaskCreate(vTreadTask,
                task_name,
                TREAD_STACK_SIZE, 
                (tasks + i),
                TREAD_PRIORITY,
                (TaskHandle_t*) NULL );
  }
  
  xTaskCreate( vEntranceTask, 
              "Entrance", 
              ENTRANCE_STACK_SIZE, 
              NULL, 
              ENTRANCE_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  xTaskCreate( vCrabTask, 
              "Crab", 
              CRAB_STACK_SIZE, 
              NULL, 
              CRAB_PRIORITY, 
              ( TaskHandle_t * ) NULL );

  xTaskCreate( vSetsTask, 
              "Sets", 
              SETS_STACK_SIZE, 
              NULL, 
              SETS_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  xTaskCreate( vDJTask, 
              "DJ", 
              DJ_STACK_SIZE, 
              NULL, 
              DJ_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
}

