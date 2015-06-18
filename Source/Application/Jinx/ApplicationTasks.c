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
  INIT = 0,
  SMOKE = 1,
  IDLE = 2,
  STEP0 = 3,
  STEP1 = 4,
  STEP2 = 5,
  STEP3 = 6,
  STEP3_1 = 7,
  STEP3_2 = 8,
  STEP4 = 9,
  STEP4_1 = 10,
  STEP4_2 = 11,
  STEP5 = 12,
  STEP6 = 13,
  STEP7 = 14,
  ERR_ORDER = 15,
  ERR_MINE = 16,
  LASER = 17,
  ERR_LASER = 18,
  ACTION = 19,
  CAST = 20,
  FINISH = 21
};
/* Private macro -------------------------------------------------------------*/
#define JUMP_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define JUMP_PRIORITY			  (tskIDLE_PRIORITY + 1)

#define DOOR_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define DOOR_PRIORITY			  (tskIDLE_PRIORITY + 1)

#define TREAD_STACK_SIZE    (configMINIMAL_STACK_SIZE * 2)
#define TREAD_PRIORITY      (tskIDLE_PRIORITY + 2)


#define PYRO1_PORT          (GPIOA)
#define PYRO1_PIN           (GPIO_Pin_1)
#define PYRO1_STATUS()      GPIO_ReadInputDataBit(PYRO1_PORT, PYRO1_PIN)
#define ENABLE_PYRO1_IT()   {EXTI->IMR |= PYRO1_PIN;}
#define DISABLE_PYRO1_IT()  {EXTI->IMR &= (~PYRO1_PIN);}

#define CRAB_PORT           (GPIOA)
#define CRAB_PIN            (GPIO_Pin_0)
#define CRAB_STATUS()       GPIO_ReadInputDataBit(CRAB_PORT, CRAB_PIN)
#define ENABLE_CRAB_IT()    {EXTI->IMR |= CRAB_PIN;}
#define DISABLE_CRAB_IT()   {EXTI->IMR &= (~CRAB_PIN);}

#define LASER_REV_PORT      (GPIOA)
#define LASER_REV_PIN       (GPIO_Pin_2)
#define LASER_REV_STATUS()  GPIO_ReadInputDataBit(LASER_REV_PORT, LASER_REV_PIN)
#define ENABLE_LASER_REV_IT()   {EXTI->IMR |= LASER_REV_PIN;}
#define DISABLE_LASER_REV_IT()  {EXTI->IMR &= (~LASER_REV_PIN);}

#define PYRO2_PORT          (GPIOA)
#define PYRO2_PIN           (GPIO_Pin_3)
#define PYRO2_STATUS()      GPIO_ReadInputDataBit(PYRO2_PORT, PYRO2_PIN)
#define ENABLE_PYRO2_IT()   {EXTI->IMR |= PYRO2_PIN;}
#define DISABLE_PYRO2_IT()  {EXTI->IMR &= (~PYRO2_PIN);}

#define TREAD_PORT          (GPIOA)
#define TREAD1_PIN          (GPIO_Pin_5)
#define TREAD2_PIN          (GPIO_Pin_6)
#define TREAD3_PIN          (GPIO_Pin_8)
#define TREAD4_PIN          (GPIO_Pin_9)
#define TREAD5_PIN          (GPIO_Pin_10)
#define TREAD6_PIN          (GPIO_Pin_11)
#define TREAD7_PIN          (GPIO_Pin_12)
#define TREAD8_PIN          (GPIO_Pin_15)
#define TREAD_PIN_ALL       (TREAD1_PIN|TREAD2_PIN|TREAD3_PIN|TREAD4_PIN| \
                             TREAD5_PIN|TREAD6_PIN|TREAD7_PIN|TREAD8_PIN)
#define TREAD_PIN_MINE      (TREAD3_PIN|TREAD5_PIN|TREAD6_PIN)
#define TREAD_PIN_ERR(x)    (~(TREAD_PIN_MINE | (x)))
#define TREAD_STATUS(x)     GPIO_ReadInputDataBit(TREAD_PORT, (x))
#define ENABLE_TREAD_IT(x)  {EXTI->IMR |= (x);}
#define DISABLE_TREAD_IT(x) {EXTI->IMR &= (~(x));}

/* Output pins */
#define SMOKE_PORT          (GPIOB)
#define SMOKE_PIN           (GPIO_Pin_0)
#define SMOKE_OFF()         {GPIO_SetBits(SMOKE_PORT, SMOKE_PIN);}
#define SMOKE_ON()          {GPIO_ResetBits(SMOKE_PORT, SMOKE_PIN);}

#define LASER_TX_PORT       (GPIOB)
#define LASER_TX_PIN        (GPIO_Pin_1)
#define LASER_TX_OFF()      {GPIO_SetBits(LASER_TX_PORT, LASER_TX_PIN);}
#define LASER_TX_ON()       {GPIO_ResetBits(LASER_TX_PORT, LASER_TX_PIN);}

#define BOX_CRAB_PORT       (GPIOB)
#define BOX_CRAB_PIN        (GPIO_Pin_2)
#define BOX_CRAB_OFF()      {GPIO_SetBits(BOX_CRAB_PORT, BOX_CRAB_PIN);}
#define BOX_CRAB_ON()       {GPIO_ResetBits(BOX_CRAB_PORT, BOX_CRAB_PIN);}

#define BOX_CAST_PORT       (GPIOB)
#define BOX_CAST_PIN        (GPIO_Pin_3)
#define BOX_CAST_OFF()      {GPIO_SetBits(BOX_CAST_PORT, BOX_CAST_PIN);}
#define BOX_CAST_ON()       {GPIO_ResetBits(BOX_CAST_PORT, BOX_CAST_PIN);}

#define DOOR_PORT           (GPIOB)
#define DOOR_PIN            (GPIO_Pin_4)
#define DOOR_OFF()          {GPIO_SetBits(DOOR_PORT, DOOR_PIN);}
#define DOOR_ON()           {GPIO_ResetBits(DOOR_PORT, DOOR_PIN);}

#define LAMP_PORT           (GPIOB)
#define LAMP1_PIN           (GPIO_Pin_5)
#define LAMP2_PIN           (GPIO_Pin_9)
#define LAMP3_PIN           (GPIO_Pin_10)
#define LAMP4_PIN           (GPIO_Pin_11)
#define LAMP_PIN_ALL        (LAMP1_PIN|LAMP2_PIN|LAMP3_PIN|LAMP4_PIN)
#define LAMP_OFF(x)         {GPIO_SetBits(LAMP_PORT, (x));}
#define LAMP_ON(x)          {GPIO_ResetBits(LAMP_PORT, (x));}
/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
/* The queue used to hold step trod */
QueueHandle_t xTreadQueue;
SemaphoreHandle_t xLaserSemaphore = NULL;
SemaphoreHandle_t xCrabSemaphore = NULL;
SemaphoreHandle_t xPyroSemaphore = NULL;
SemaphoreHandle_t xTreadSemaphore[8];
/* Used for pyroelectric detector */
SemaphoreHandle_t xEntranceSemaphore = NULL;

uint16_t treads[8] = {TREAD1_PIN, TREAD2_PIN, TREAD3_PIN, TREAD4_PIN,
                      TREAD5_PIN, TREAD6_PIN, TREAD7_PIN, TREAD8_PIN};
uint8_t tasks[8] = {0, 1, 2, 3, 4, 5, 6, 7};
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION( vJumpTask, pvParameters ) {
  int status = INIT;
  IT_event tread_event;
  TickType_t tick_tmp;
  TickType_t tick_total;
  TickType_t tick_delay;
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        status = SMOKE;
        LAMP_OFF(LAMP_PIN_ALL);
        SMOKE_OFF();
        DISABLE_PYRO1_IT();
        while (pdTRUE == xSemaphoreTake(xEntranceSemaphore, 0));
        DISABLE_TREAD_IT(TREAD_PIN_ALL);
        while (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, 0));
        /* Disable the laser reciever interrupt */
        DISABLE_LASER_REV_IT();
        /* Shut down the laser transmitter */
        LASER_TX_OFF();
        while (pdTRUE == xSemaphoreTake(xLaserSemaphore, 0));
        BOX_CRAB_OFF();
        /* Enable entey pyro detector */
        ENABLE_PYRO1_IT();
        break;
      }
      case SMOKE: {
        if (pdTRUE == xSemaphoreTake(xEntranceSemaphore, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          if (Bit_RESET != PYRO1_STATUS()) {
            /* Play music */
            player_play_file(ENTRANCE_AUDIO, 0);
            SMOKE_ON();
            vTaskDelay((TickType_t)SMOKE_MS);
            SMOKE_OFF();
            status = IDLE;
          } else {
            ENABLE_PYRO1_IT();
          }
        }
        break;
      }
      case IDLE: {
        DISABLE_TREAD_IT(TREAD_PIN_ALL);
        while (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, 0));
        ENABLE_TREAD_IT(TREAD_PIN_ALL);
        status = STEP0;
      }
      case STEP0:
      case STEP7: {
        if (STEP0 == status) {
          tick_delay = portMAX_DELAY;
        } else {
          tick_delay = MAX_TREAD_MS - (xTaskGetTickCount() - tick_total);
        }
        
        if (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, tick_delay)) {
          if (STEP0 == status) {
            /* Record the current time */
            tick_total = xTaskGetTickCount();
          }
          if (treads[tread_event.event] & TREAD_PIN_MINE) {
            status = ERR_MINE;
            break;
          } else if (treads[tread_event.event] & TREAD_PIN_ERR(TREAD1_PIN)) {
            status = ERR_ORDER;
            break;
          } else {
            if (STEP0 == status) {
              player_play_file(CORRECT_AUDIO, 0);
              status = STEP1;
            } else {
              player_play_file(TREAD_COMPLETE_AUDIO, 0);
              status = LASER;
            }
            ENABLE_TREAD_IT(TREAD_PIN_ALL);
          }
        } else {
          status = ERR_ORDER;
        }
        break;
      }
      case STEP1:
      case STEP6: {
        tick_delay = MAX_TREAD_MS - (xTaskGetTickCount() - tick_total);
        if (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, tick_delay)) {
          if (treads[tread_event.event] & TREAD_PIN_MINE) {
            status = ERR_MINE;
            break;
          } else if (treads[tread_event.event] & TREAD_PIN_ERR(TREAD2_PIN)) {
            status = ERR_ORDER;
            break;
          } else {
            player_play_file(CORRECT_AUDIO, 0);
            if (STEP1 == status) {
              status = STEP2;
            } else {
              status = STEP7;
            }
            ENABLE_TREAD_IT(TREAD_PIN_ALL);
          }
        } else {
          status = ERR_ORDER;
        }
        break;
      }
      case STEP2:
      case STEP5: {
        tick_delay = MAX_TREAD_MS - (xTaskGetTickCount() - tick_total);
        if (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, tick_delay)) {
          if (treads[tread_event.event] & TREAD_PIN_MINE) {
            status = ERR_MINE;
            break;
          } else if (treads[tread_event.event] & TREAD_PIN_ERR(TREAD4_PIN)) {
            status = ERR_ORDER;
            break;
          } else {
            player_play_file(CORRECT_AUDIO, 0);
            if (STEP2 == status) {
              status = STEP3;
            } else { 
              status = STEP6;
            }
            ENABLE_TREAD_IT(TREAD_PIN_ALL);
          }
        } else {
          status = ERR_ORDER;
        }
        break;
      }
      case STEP3:
      case STEP4: {
        tick_delay = MAX_TREAD_MS - (xTaskGetTickCount() - tick_total);
        if (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, tick_delay)) {
            if (treads[tread_event.event] & TREAD_PIN_MINE) {
              status = ERR_MINE;
              break;
            } else if (treads[tread_event.event] & 
                                       TREAD_PIN_ERR(TREAD7_PIN | TREAD8_PIN)) {
              status = ERR_ORDER;
              break;
            } else if (treads[tread_event.event] & TREAD7_PIN) {
              tick_tmp = tread_event.time_stamp;
              if (STEP3 == status) {
                status = STEP3_1;
              } else {
                status = STEP4_1;
              }
              ENABLE_TREAD_IT(TREAD_PIN_ALL);
            } else {
              tick_tmp = tread_event.time_stamp;
              if (STEP3 == status) {
                status = STEP3_2;
              } else {
                status = STEP4_2;
              }
              ENABLE_TREAD_IT(TREAD_PIN_ALL);
            }
        } else {
          status = ERR_ORDER;
        }
        break;
      }
      case STEP3_1:
      case STEP4_1: {
        tick_delay = MAX_TREAD_MS - (xTaskGetTickCount() - tick_total);
        if (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, tick_delay)) {
          if (treads[tread_event.event] & TREAD_PIN_MINE) {
            status = ERR_MINE;
            break;
          } else if (treads[tread_event.event] & TREAD_PIN_ERR(TREAD8_PIN)) {
            status = ERR_ORDER;
            break;
          } else {
            if ((tread_event.time_stamp - tick_tmp) <= TWO_FEET_INTERVAL_MS) {
              player_play_file(CORRECT_AUDIO, 0);
              if (STEP3_1 == status) {
                status = STEP4;
              } else {
                status = STEP5;
              }
              ENABLE_TREAD_IT(TREAD_PIN_ALL);
            } else {
              status = ERR_ORDER;
              break;
            }
          }
        } else {
          status = ERR_ORDER;
        }
        break;
      }
      case STEP3_2:
      case STEP4_2: {
        tick_delay = MAX_TREAD_MS - (xTaskGetTickCount() - tick_total);
        if (pdTRUE == xQueueReceive(xTreadQueue, &tread_event, tick_delay)) {
          if (treads[tread_event.event] & TREAD_PIN_MINE) {
            status = ERR_MINE;
            break;
          } else if (treads[tread_event.event] & TREAD_PIN_ERR(TREAD7_PIN)) {
            status = ERR_ORDER;
            break;
          } else {
            if ((tread_event.time_stamp - tick_tmp) < TWO_FEET_INTERVAL_MS) {
              player_play_file(CORRECT_AUDIO, 0);
              if (STEP3_2 == status) {
                status = STEP4;
              } else {
                status = STEP5;
              }
              ENABLE_TREAD_IT(TREAD_PIN_ALL);
            } else {
              status = ERR_ORDER;
              break;
            }
          }
          ENABLE_TREAD_IT(TREAD_PIN_ALL);
        } else {
          status = ERR_ORDER;
        }
        break;
      }
      case LASER: {
        /* Turn on laser trasmitter */
        LASER_TX_ON();
        /* Wait for the laser is stable */
        vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
        /* Flush the xLaserSemaphore Semaphore */
        while (pdTRUE == xSemaphoreTake(xLaserSemaphore, 0));
        /* Check the reciever status */
        if (LASER_REV_STATUS()) {
          player_play_file(LASER_CORRECT_AUDIO, 0);
          /* Reciever has caught the laser ray */
          status = FINISH;
          vTaskDelay((TickType_t)KEEP_LASER_ON_MS);
          /* Open the crab box */
          BOX_CRAB_ON();
        } else {
          /* Enable laser reciever interrupt */
          ENABLE_LASER_REV_IT();
          /* Reciever has not caught the laser ray */
          if (pdTRUE == xSemaphoreTake(xLaserSemaphore, ESCAPE_LASER_MS)) {
            player_play_file(LASER_CORRECT_AUDIO, 0);
            status = FINISH;
            vTaskDelay((TickType_t)KEEP_LASER_ON_MS);
            /* Open the crab box */
            BOX_CRAB_ON();
          } else {
            status = ERR_LASER;
          }
        }
        /* Shut down the laser transmitter */
        LASER_TX_OFF();
        break;
      }
      case ERR_LASER: {
        /* Play music */
        player_play_file(LASER_ERR_AUDIO, 0);
        status = IDLE;
        break;
      }
      case ERR_MINE: {
        /* Play music */
        player_play_file(MINE_AUDIO, 0);
        /* Flash the red lamp */
        LAMP_ON(LAMP_PIN_ALL);
        vTaskDelay((TickType_t)LAMP_ERR_FLASH_MS);
        LAMP_OFF(LAMP_PIN_ALL);
        status = IDLE;
        break;
      }
      case ERR_ORDER: {
        /* Play music */
        player_play_file(ORDER_ERR_AUDIO, 0);
        /* Flash the red lamp */
        LAMP_ON(LAMP_PIN_ALL);
        vTaskDelay((TickType_t)LAMP_ERR_FLASH_MS);
        LAMP_OFF(LAMP_PIN_ALL);
        status = IDLE;
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

static portTASK_FUNCTION( vDoorTask, pvParameters ) {
  int status = INIT;

	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        DOOR_OFF();
        BOX_CAST_OFF();
        /* Enable crab remove interrupt */
        DISABLE_CRAB_IT();
        while (pdTRUE == xSemaphoreTake(xCrabSemaphore, 0));
        ENABLE_CRAB_IT();
        /* Enable pyro interrupt */
        DISABLE_PYRO2_IT();
        while (pdTRUE == xSemaphoreTake(xPyroSemaphore, 0));
        status = IDLE;
        break;
      }
      case IDLE: {
        /* Occur when the crab is removed */        
        if (xSemaphoreTake(xCrabSemaphore, portMAX_DELAY) == pdTRUE) {
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
        /* Open the door to the next room */
        DOOR_ON();
        /* Enable pyro detector interrupt */
        ENABLE_PYRO2_IT();
        status = CAST;
        break;
      }
      case CAST: {
        if (pdTRUE == xSemaphoreTake(xPyroSemaphore, portMAX_DELAY)) {
          /* Open the cast box */
          BOX_CAST_ON();
          status = FINISH;
        }
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

static portTASK_FUNCTION(vTreadTask, pvParameters) {
  int index = *((char*)pvParameters);
  IT_event event;
  
  event.event = index;
  for(;;) {
    if (pdTRUE == xSemaphoreTake(xTreadSemaphore[index], portMAX_DELAY)) {
      /* Skip the key jitter step */
      vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
      if (Bit_RESET == TREAD_STATUS(treads[index])) {
        event.time_stamp = xTaskGetTickCountFromISR();
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
  GPIO_InitStructure.GPIO_Pin = PYRO1_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PYRO1_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = CRAB_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CRAB_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = LASER_REV_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LASER_REV_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = PYRO2_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PYRO2_PORT, &GPIO_InitStructure);  
  
  GPIO_InitStructure.GPIO_Pin = TREAD_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(TREAD_PORT, &GPIO_InitStructure);
  
  /* Configure output GPIO */
  SMOKE_OFF();
  GPIO_InitStructure.GPIO_Pin = SMOKE_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SMOKE_PORT, &GPIO_InitStructure);
  
  LASER_TX_OFF();
  GPIO_InitStructure.GPIO_Pin = LASER_TX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LASER_TX_PORT, &GPIO_InitStructure);

  BOX_CRAB_OFF();
  GPIO_InitStructure.GPIO_Pin = BOX_CRAB_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BOX_CRAB_PORT, &GPIO_InitStructure);
  
  BOX_CAST_OFF();
  GPIO_InitStructure.GPIO_Pin = BOX_CAST_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BOX_CAST_PORT, &GPIO_InitStructure);

  DOOR_OFF();
  GPIO_InitStructure.GPIO_Pin = DOOR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DOOR_PORT, &GPIO_InitStructure);
  
  LAMP_OFF(LAMP_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LAMP_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LAMP_PORT, &GPIO_InitStructure);
  
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
  
	EXTI_InitStructure.EXTI_Line = CRAB_PIN|PYRO1_PIN|PYRO2_PIN|LASER_REV_PIN;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);  
	
	EXTI_InitStructure.EXTI_Line = TREAD_PIN_ALL;
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
  
  /* Init timer 2 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  
}

void init_tasks(void) {
  char i;
  char task_name[configMAX_TASK_NAME_LEN] = {'T','r','e','a','d','1',0};
  
  /* Initialize hardware */
  hardware_init();

  /* Create reset semaphore */
  xResetSemaphore = xSemaphoreCreateBinary();

  /* Create entre detector semaphore */
  xEntranceSemaphore = xSemaphoreCreateBinary();
  
  /* Create tread queue */
  xTreadQueue = xQueueCreate(20, (unsigned portBASE_TYPE)sizeof(IT_event));  
  
  for (i = 0; i < 8; i++) {
    xTreadSemaphore[i] = xSemaphoreCreateBinary();
    task_name[5] = i + '1';
    xTaskCreate(vTreadTask,
                task_name,
                TREAD_STACK_SIZE, 
                (tasks + i),
                TREAD_PRIORITY,
                (TaskHandle_t*) NULL );
  } 
  
  /* Create laser semaphore */
  xLaserSemaphore = xSemaphoreCreateBinary();
  
  xTaskCreate( vJumpTask, 
              "Jump", 
              JUMP_STACK_SIZE, 
              NULL, 
              JUMP_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  /* Create crab remove semaphore */
  xCrabSemaphore = xSemaphoreCreateBinary();
  
  /* Create pyro detector semaphore */
  xPyroSemaphore = xSemaphoreCreateBinary();
  
  xTaskCreate( vDoorTask, 
              "Door", 
              DOOR_STACK_SIZE, 
              NULL, 
              DOOR_PRIORITY, 
              ( TaskHandle_t * ) NULL );
}

