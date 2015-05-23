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
  SHOOTING,
  LASER,
  ESCAPE_LASER,
  ENABLE_BUTTON,
  BUTTON,
  INIT_PAINTING,
  PAINTING,
  ERR_LASER,
  
  CRAB_BOX,
  WAIT_ACTIVE,
  ACTION,
  CAST,
  FINISH
};
/* Private macro -------------------------------------------------------------*/
#define DRAGON_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define DRAGON_PRIORITY			  (tskIDLE_PRIORITY + 1 )

#define PAINTING_STACK_SIZE   (configMINIMAL_STACK_SIZE * 3)
#define PAINTING_PRIORITY     (tskIDLE_PRIORITY + 1)

#define PYRO_PORT             (GPIOA)
#define PYRO_PIN              (GPIO_Pin_0)
#define PYRO_STATUS()         GPIO_ReadInputDataBit(PYRO_PORT, PYRO_PIN)
#define ENABLE_PYRO_IT()      {EXTI->IMR |= PYRO_PIN;}
#define DISABLE_PYRO_IT()     {EXTI->IMR &= (~PYRO_PIN);}

#define BUTTON_PORT           (GPIOA)
#define BUTTON1_PIN           (GPIO_Pin_1)
#define BUTTON2_PIN           (GPIO_Pin_2)
#define BUTTON3_PIN           (GPIO_Pin_3)
#define BUTTON4_PIN           (GPIO_Pin_4)
#define BUTTON_PIN_ALL        (BUTTON1_PIN|BUTTON2_PIN|BUTTON3_PIN|BUTTON4_PIN)
#define BUTTON_STATUS(x)      GPIO_ReadInputDataBit(BUTTON_PORT, (x))
#define ENABLE_BUTTON_IT(x)   {EXTI->IMR |= (x);}
#define DISABLE_BUTTON_IT(x)  {EXTI->IMR &= (~(x));}

#define LASER_RX_PORT         (GPIOB)
#define LASER_RX1_PIN         (GPIO_Pin_5)
#define LASER_RX2_PIN         (GPIO_Pin_6)
#define LASER_RX3_PIN         (GPIO_Pin_7)
#define LASER_RX4_PIN         (GPIO_Pin_8)
#define LASER_RX_PIN_ALL      (LASER_RX1_PIN|LASER_RX2_PIN| \
                               LASER_RX3_PIN|LASER_RX4_PIN)
#define LASER_RX_STATUS(x)    GPIO_ReadInputDataBit(LASER_RX_PORT, (x))
#define ENABLE_LASER_RX_IT(x)   {EXTI->IMR |= (x);}
#define DISABLE_LASER_RX_IT(x)  {EXTI->IMR &= ~(x);}

#define LASER_ESC_RX_PORT     (GPIOE)
#define LASER_ESC_RX_PIN      (GPIO_Pin_9)

#define HALL_PORT             (GPIOE)
#define HALL1_PIN             (GPIO_Pin_10)
#define HALL2_PIN             (GPIO_Pin_11)
#define HALL3_PIN             (GPIO_Pin_12)
#define HALL4_PIN             (GPIO_Pin_13)
#define HALL5_PIN             (GPIO_Pin_14)
#define HALL6_PIN             (GPIO_Pin_15)
#define HALL_SWORD_PIN        (GPIO_Pin_8)
#define HALL_PIN_ALL          (HALL1_PIN|HALL2_PIN|HALL3_PIN| \
                               HALL4_PIN|HALL5_PIN|HALL6_PIN)
#define HALL_STATUS(x)        GPIO_ReadInputDataBit(HALL_PORT, (x))
#define ENABLE_HALL_IT(x)     {EXTI->IMR |= (x);}
#define DISABLE_HALL_IT(x)    {EXTI->IMR &= (~(x));}

#define SWITCH_STATUS(x)      GPIO_ReadInputDataBit(HALL_PORT, (1 << (x)))
#define ENABLE_SWITCH_IT(x)   {EXTI->IMR |= ((HALL1_PIN >> 1) << (x));}

#define PLAYER_BUSY_PORT      (GPIOA)
#define PLAYER_BUSY_PIN       (GPIO_Pin_8)
#define PLAYER_BUSY_STATUS()  (GPIO_ReadInputDataBit(PLAYER_BUSY_PORT, \
                                                     PLAYER_BUSY_PIN))

#define USART1_PORT           (GPIOA)
#define USART1_TX_PIN         (GPIO_Pin_9)
#define USART1_RX_PIN         (GPIO_Pin_10)

/* Output pins */
#define LASER_TX_PORT         (GPIOB)
#define LASER_TX1_PIN         (GPIO_Pin_10)
#define LASER_TX2_PIN         (GPIO_Pin_11)
#define LASER_TX3_PIN         (GPIO_Pin_12)
#define LASER_TX4_PIN         (GPIO_Pin_13)
#define LASER_TX5_PIN         (GPIO_Pin_14)
#define LASER_TX6_PIN         (GPIO_Pin_15)
#define LASER_TX_PIN_ALL      (LASER_TX1_PIN|LASER_TX2_PIN|LASER_TX3_PIN| \
                               LASER_TX4_PIN|LASER_TX5_PIN|LASER_TX6_PIN)
#define LASER_TX_OFF(x)       {GPIO_SetBits(LASER_TX_PORT, (x));}
#define LASER_TX_ON(x)        {GPIO_ResetBits(LASER_TX_PORT, (x));}

#define LED_BUTTON_PORT       (GPIOC)
#define LED_BUTTON1_PIN       (GPIO_Pin_6)
#define LED_BUTTON2_PIN       (GPIO_Pin_7)
#define LED_BUTTON3_PIN       (GPIO_Pin_8)
#define LED_BUTTON4_PIN       (GPIO_Pin_9)
#define LED_BUTTON_PIN_ALL    (LED_BUTTON1_PIN|LED_BUTTON2_PIN| \
                               LED_BUTTON3_PIN|LED_BUTTON4_PIN)
#define LED_BUTTON_OFF(x)     {GPIO_SetBits(LED_BUTTON_PORT, (x));}
#define LED_BUTTON_ON(x)      {GPIO_ResetBits(LED_BUTTON_PORT, (x));}

#define LED_T1_PORT           (GPIOD)
#define LED_T1_1_PIN          (GPIO_Pin_8)
#define LED_T1_2_PIN          (GPIO_Pin_9)
#define LED_T1_3_PIN          (GPIO_Pin_10)
#define LED_T1_4_PIN          (GPIO_Pin_11)
#define LED_T1_5_PIN          (GPIO_Pin_12)
#define LED_T1_PIN_ALL        (LED_T1_1_PIN|LED_T1_2_PIN|LED_T1_3_PIN| \
                               LED_T1_4_PIN|LED_T1_5_PIN)
#define LED_T1_OFF(x)         {GPIO_SetBits(LED_T1_PORT, (x));}
#define LED_T1_ON(x)          {GPIO_ResetBits(LED_T1_PORT, (x));}

#define LED_T2_PORT           (GPIOD)
#define LED_T2_1_PIN          (GPIO_Pin_0)
#define LED_T2_2_PIN          (GPIO_Pin_1)
#define LED_T2_3_PIN          (GPIO_Pin_2)
#define LED_T2_4_PIN          (GPIO_Pin_3)
#define LED_T2_5_PIN          (GPIO_Pin_4)
#define LED_T2_PIN_ALL        (LED_T2_1_PIN|LED_T2_2_PIN|LED_T2_3_PIN| \
                               LED_T2_4_PIN|LED_T2_5_PIN)
#define LED_T2_OFF(x)         {GPIO_SetBits(LED_T2_PORT, (x));}
#define LED_T2_ON(x)          {GPIO_ResetBits(LED_T2_PORT, (x));}

#define LED_F1_PORT           (GPIOD)
#define LED_F1_1_PIN          (GPIO_Pin_13)
#define LED_F1_2_PIN          (GPIO_Pin_14)
#define LED_F1_3_PIN          (GPIO_Pin_15)
#define LED_F1_PIN_ALL        (LED_F1_1_PIN|LED_F1_2_PIN|LED_F1_3_PIN)
#define LED_F1_OFF(x)         {GPIO_SetBits(LED_F1_PORT, (x));}
#define LED_F1_ON(x)          {GPIO_ResetBits(LED_F1_PORT, (x));}

#define LED_F2_PORT           (GPIOD)
#define LED_F2_1_PIN          (GPIO_Pin_5)
#define LED_F2_2_PIN          (GPIO_Pin_6)
#define LED_F2_3_PIN          (GPIO_Pin_7)
#define LED_F2_PIN_ALL        (LED_F2_1_PIN|LED_F2_2_PIN|LED_F2_3_PIN)
#define LED_F2_OFF(x)         {GPIO_SetBits(LED_F2_PORT, (x));}
#define LED_F2_ON(x)          {GPIO_ResetBits(LED_F2_PORT, (x));}

#define LED_T_F_PORT          (GPIOD)
#define LED_T_F_PIN_ALL       GPIO_Pin_All
#define LED_T_F_OFF(x)        {GPIO_SetBits(LED_T_F_PORT, (x));}
#define LED_T_F_ON(x)         {GPIO_ResetBits(LED_T_F_PORT, (x));}

#define LAMP_CAST_PORT        (GPIOE)
#define LAMP_CAST_PIN         (GPIO_Pin_0)
#define LAMP_CAST_OFF()       {GPIO_SetBits(LAMP_CAST_PORT, LAMP_CAST_PIN);}
#define LAMP_CAST_ON()        {GPIO_ResetBits(LAMP_CAST_PORT, LAMP_CAST_PIN);}

#define BOX_CHEST_PORT        (GPIOE)
#define BOX_CHEST_PIN         (GPIO_Pin_2)
#define BOX_CHEST_OFF()       {GPIO_SetBits(BOX_CHEST_PORT, BOX_CHEST_PIN);}
#define BOX_CHEST_ON()        {GPIO_ResetBits(BOX_CHEST_PORT, BOX_CHEST_PIN);}

#define BOX_SWORD_PORT        (GPIOE)
#define BOX_SWORD_PIN         (GPIO_Pin_4)
#define BOX_SWORD_OFF()       {GPIO_SetBits(BOX_SWORD_PORT, BOX_SWORD_PIN);}
#define BOX_SWORD_ON()        {GPIO_ResetBits(BOX_SWORD_PORT, BOX_SWORD_PIN);}

#define DOOR_PORT             (GPIOE)
#define DOOR_PIN              (GPIO_Pin_6)
#define DOOR_OFF()            {GPIO_SetBits(DOOR_PORT, DOOR_PIN);}
#define DOOR_ON()             {GPIO_ResetBits(DOOR_PORT, DOOR_PIN);}

#define LAMP_PORT             (GPIOC)
#define LAMP_PIN              (GPIO_Pin_14)
#define LAMP_OFF()           {GPIO_SetBits(LAMP_PORT, LAMP_PIN);}
#define LAMP_ON()            {GPIO_ResetBits(LAMP_PORT, LAMP_PIN);}
/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
/* Used for pyroelectric detector */
SemaphoreHandle_t xEntranceSemaphore = NULL;
/* The queue used to hold shooting event */
QueueHandle_t xShootingQueue;
/* The queue used to hold buttons event */
QueueHandle_t xButtonQueue;
/* The queue used to hold received characters. */
QueueHandle_t xSwitchQueue;
/* Used for active painting task */
SemaphoreHandle_t xPaintingSemaphore = NULL;

uint16_t led_button[4] = {LED_BUTTON1_PIN, LED_BUTTON2_PIN, 
                          LED_BUTTON3_PIN, LED_BUTTON4_PIN};
uint16_t button[4] = {BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN};
static uint8_t blood[4] = {BLOOD_TRUE_EYE, BLOOD_TRUE_EYE, 
                           BLOOD_FALSE_EYE, BLOOD_FALSE_EYE};


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

int random_button(void) {
  int result;
  
  result = rand() % (blood[0] + blood[1] + blood[2] + blood[3]);
  
  if (0 == result)
    return -1;
  
  if (blood[0]) {
    if (result < blood[0]) {
      blood[0]--;
      return 0;
    }
  }
  if (blood[1]) {
    if (result < (blood[0] + blood[1])) {
      blood[1]--;
      return 1;
    }
  }
  if (blood[2]) {
    if (result < (blood[0] + blood[1] + blood[2])) {
      blood[2]--;
      return 2;
    }
  }
  blood[3]--;
  return 3;
}

void led_button_ctl(void) {
}

void led_button_reset(void) {
  blood[0] = blood[1] = BLOOD_TRUE_EYE; 
  blood[2] = blood[3] = BLOOD_FALSE_EYE;
}

static portTASK_FUNCTION( vDragonTask, pvParameters ) {
  int status = INIT;
  IT_event event;
  TickType_t tick_delay;
  uint16_t laser_rx_flag;
  TickType_t tick_escape_laser;
  TickType_t tick_previous_event;
    
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        /* TIM enable counter */
        TIM_Cmd(TIM1, ENABLE);
        /* Turn off the LED on the buton */
        LED_BUTTON_OFF(LED_BUTTON_PIN_ALL);
        /* Turn off the blood LEDs */
        LED_T_F_OFF(LED_T_F_PIN_ALL);
        /* Turn off the 6 laser transmitter */
        LASER_TX_OFF(LASER_TX_PIN_ALL);
        /* Turn off the cast lamp */
        LAMP_CAST_OFF();
        /* Close the chest box */
        BOX_CHEST_OFF();
        /* Clean buttons interrupt */
        DISABLE_BUTTON_IT(BUTTON_PIN_ALL);
        while (pdTRUE == xQueueReceive(xButtonQueue, &event, 0));
        /* Clean shooting laser reciever interrupt */
        laser_rx_flag = 0;
        DISABLE_LASER_RX_IT(LASER_RX_PIN_ALL);
        while (pdTRUE == xQueueReceive(xShootingQueue, &event, 0));
        /* Clean pyroelectric detector interrupt */
        DISABLE_PYRO_IT();
        while (pdTRUE == xSemaphoreTake(xEntranceSemaphore, 0));
        ENABLE_PYRO_IT();
        status = IDLE;
        break;
      }
      case IDLE: {
        if (pdTRUE == xSemaphoreTake(xEntranceSemaphore, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          if (Bit_RESET != PYRO_STATUS()) {
            /* Play audio */
            player_play_file(ENTRANCE_AUDIO, 0);
            ENABLE_LASER_RX_IT(LASER_RX_PIN_ALL);
            status = SHOOTING;
          } else {
            ENABLE_PYRO_IT();
          }
        }
        break;
      }
      case SHOOTING: {
        if (pdTRUE == xQueueReceive(xShootingQueue, &event, portMAX_DELAY)) {
          /* Skip the key jitter step */
          vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
          event.event &= LASER_RX_PIN_ALL;
          if (Bit_RESET != LASER_RX_STATUS(event.event)) {
            /* Play audio */
            player_play_file(SHOOTING_AUDIO, 0);
            laser_rx_flag |= event.event;
            if (LASER_RX_PIN_ALL == (laser_rx_flag & LASER_RX_PIN_ALL)) {
              status = LASER;
            }
          } else {
            ENABLE_LASER_RX_IT(LASER_RX_PIN_ALL);
          }
        }
        break;
      }
      case LASER: {
        /* Play audio */
        player_play_file(LASER_AUDIO, 0);
        /* Wait for audio ending */
        while(Bit_RESET != PLAYER_BUSY_STATUS());
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        /* Turn on the 6 laser transmitter */
        LASER_TX_ON(LASER_TX_PIN_ALL);
        /* Wait for the laser ray stable */
        vTaskDelay((TickType_t)10);
//        /* Enable the laser switch reciever interrupt */
//        ENABLE_LASER_RX_IT(LASER_RX_SWITCH_PIN);
//        /* Record the current time for the player escaping the laser ray */
//        tick_previous_event = tick_escape_laser = xTaskGetTickCount();
        status = ESCAPE_LASER;
        break;
      }
      case ESCAPE_LASER: {
        tick_delay = tick_escape_laser + ESCAPE_LASER_MS - xTaskGetTickCount();
        if (pdTRUE == xQueueReceive(xShootingQueue, &event, tick_delay)) {
//          if (LASER_RX_SWITCH_PIN == event.event & LASER_RX_SWITCH_PIN;) {
//            if (Bit_RESET == LASER_RX_STATUS(LASER_RX_SWITCH_PIN)) {
//              /* The laser ray has been blocked */
//              
//            } else {
//              /* The laser ray has been released */
//            }
//          } else {
//            /* Enable the laser switch reciever interrupt */
//            ENABLE_LASER_RX_IT(LASER_RX_SWITCH_PIN);
//          }
        } else {
        }
        break;
      }
      case ENABLE_BUTTON: {
        /* Play audio */
        player_play_file(BUTTON_AUDIO, 0);
        /* LEDs show */
        LED_BUTTON_ON(LED_BUTTON_PIN_ALL);
        LED_T_F_ON(LED_T_F_PIN_ALL);
        vTaskDelay((TickType_t)1000);
        LED_BUTTON_OFF(LED_BUTTON_PIN_ALL);
        /* Reset eyes' blood */
        led_button_reset();
        /* Init random seed */
        srand(xTaskGetTickCount());
        status = BUTTON;
        break;
      }
      case BUTTON: {
        int index, j;
        /* A random button */
        index = random_button();
        /* Enable button interrupt */
        ENABLE_BUTTON_IT(BUTTON_PIN_ALL);
        /* Check whether the button is vailed */
        if (index != -1) {
          /* At least one eye has blood */
          LED_BUTTON_ON(led_button[index]);
          if (xQueueReceive(xButtonQueue, &event, LED_BUTTON_ON_DELAY_MS)) {
            /* Skip the key jitter step */
            vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
            event.event &= BUTTON_PIN_ALL;
            if (Bit_RESET == BUTTON_STATUS(event.event)) {
              if (button[index] == (event.event & button[index])) {
                if (0 != blood[index])
                  blood[index]--;
              } else {
                for (j = 0; j < 4; j++) {
                  if ((j != index) && 
                   (button[j] == (event.event & button[j])) && 
                   (((j < 2) ? BLOOD_TRUE_EYE : BLOOD_FALSE_EYE) != blood[j])) {
                    blood[j]++;
                  }
                }
              }
              led_button_ctl();
            }
          }
          LED_BUTTON_OFF(led_button[index]);
        } else {
          /* All eyes has no blood */
          LED_BUTTON_OFF(BUTTON_PIN_ALL);
          LASER_TX_OFF(LASER_TX_PIN_ALL);
          status = INIT_PAINTING;
        }
        break;
      }
      case INIT_PAINTING: {
        /* Turn on the cast lamp */
        LAMP_CAST_ON();
        /* Close the chest box */
        BOX_CHEST_ON();
        /* Clean painting semaphore */
        xSemaphoreTake(xPaintingSemaphore, 0);
        /* Active painting task */
        xSemaphoreGive(xPaintingSemaphore);
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

static portTASK_FUNCTION( vPaintingTask, pvParameters ) {
  int status = INIT;
  char switch_no;
  char switch_passed;
  
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case WAIT_ACTIVE: {
        /* Clean hall interrupt queue */
        DISABLE_HALL_IT(HALL_PIN_ALL);
        if (pdTRUE == xSemaphoreTake(xPaintingSemaphore, portMAX_DELAY)) {
          status = INIT;
        }
        break;
      }
      case INIT: {
        switch_passed = 0;
        BOX_SWORD_OFF();
        DOOR_OFF();
        /* Clean hall interrupt queue */
        DISABLE_HALL_IT(HALL_PIN_ALL);
        while (xQueueReceive(xSwitchQueue, &switch_no, 0));
        ENABLE_HALL_IT(HALL1_PIN);
        status = IDLE;
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
        DISABLE_HALL_IT(HALL_PIN_ALL);
        /* Play audio */
        player_play_file(SWORD_AUDIO, 0);
        /* Wait for audio ending */
        while(Bit_RESET != PLAYER_BUSY_STATUS());
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        /* Open the box */
        BOX_SWORD_ON();
        /* Wait for removing the sword */
        for (;;) {
          if (Bit_RESET != HALL_STATUS(HALL_SWORD_PIN)) {
            /* Skip the key jitter step */
            vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
            if (Bit_RESET != HALL_STATUS(HALL_SWORD_PIN)) {
              break;
            }
          }
          vTaskDelay((TickType_t)10);
        }
        /* Play audio */
        player_play_file(OPEN_DOOR_AUDIO, 0);
        /* Wait for audio ending */
        while(Bit_RESET != PLAYER_BUSY_STATUS());
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        /* Open door */
        DOOR_ON();
        status = FINISH;
        break;
      }
      case FINISH: {
        /* Reset logic */
        if (pdTRUE == xSemaphoreTake(xResetSemaphore, portMAX_DELAY)) {
          status = WAIT_ACTIVE;
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
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_ICInitTypeDef TIM_ICInitStructure;
  
  /* Configure input GPIO */
  GPIO_InitStructure.GPIO_Pin = PYRO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PYRO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = BUTTON_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BUTTON_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = LASER_RX_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LASER_RX_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = LASER_ESC_RX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LASER_ESC_RX_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = HALL_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(HALL_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = HALL_SWORD_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(HALL_PORT, &GPIO_InitStructure);
  
  /* Configure output GPIO */
  LASER_TX_OFF(LASER_TX_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LASER_TX_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LASER_TX_PORT, &GPIO_InitStructure);
  
  LED_BUTTON_OFF(LED_BUTTON_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_BUTTON_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_BUTTON_PORT, &GPIO_InitStructure);

  LED_T1_OFF(LED_T1_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_T1_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_T1_PORT, &GPIO_InitStructure);
  
  LED_T2_OFF(LED_T2_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_T2_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_T2_PORT, &GPIO_InitStructure);

  LED_F1_OFF(LED_F1_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_F1_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_F1_PORT, &GPIO_InitStructure);
  
  LED_F2_OFF(LED_F2_PIN_ALL);
  GPIO_InitStructure.GPIO_Pin = LED_F2_PIN_ALL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LED_F2_PORT, &GPIO_InitStructure);
  
  LAMP_CAST_OFF();
  GPIO_InitStructure.GPIO_Pin = LAMP_CAST_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LAMP_CAST_PORT, &GPIO_InitStructure);

  BOX_CHEST_OFF();
  GPIO_InitStructure.GPIO_Pin = BOX_CHEST_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BOX_CHEST_PORT, &GPIO_InitStructure);
  
  BOX_SWORD_OFF();
  GPIO_InitStructure.GPIO_Pin = BOX_SWORD_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BOX_SWORD_PORT, &GPIO_InitStructure);
  
  DOOR_OFF();
  GPIO_InitStructure.GPIO_Pin = DOOR_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DOOR_PORT, &GPIO_InitStructure);
  
  LAMP_OFF();
  GPIO_InitStructure.GPIO_Pin = LAMP_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LAMP_PORT, &GPIO_InitStructure);
  
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource5);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource10);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource11);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource12);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource13);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource14);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource15);
  
	EXTI_InitStructure.EXTI_Line = PYRO_PIN|LASER_RX_PIN_ALL;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);  
	
	EXTI_InitStructure.EXTI_Line = BUTTON_PIN_ALL|HALL_PIN_ALL;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
//	EXTI_InitStructure.EXTI_Line = LASER_RX_SWITCH_PIN;
//  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
//  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//  EXTI_Init(&EXTI_InitStructure);
  
  /* Enable the TIM1 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                          configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 
                          configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
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
  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
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
  
  /* Init timer 1 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
  
  /*Configure peripheral I/O remapping */
  GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);  
  
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = (ESCAPE_LASER_MS * 2);
  TIM_TimeBaseStructure.TIM_Prescaler = (36000 - 1);
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
  
  /* TIM1 configuration: PWM Input mode ------------------------
     The external signal is connected to TIM1 CH1 pin (PE.09), 
     The Rising edge is used as active edge,
     The TIM1 CCR2 is used to compute the frequency value 
     The TIM1 CCR1 is used to compute the duty cycle value
  ------------------------------------------------------------ */
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0x0;
  
  TIM_PWMIConfig(TIM1, &TIM_ICInitStructure);
  /* Select the TIM1 Input Trigger: TI1FP1 */
  TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1);
  /* Select the slave Mode: Reset Mode */
  TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);
  /* Enable the Master/Slave Mode */
  TIM_SelectMasterSlaveMode(TIM1, TIM_MasterSlaveMode_Enable);
  TIM_ClearITPendingBit(TIM1, (TIM_IT_Update | TIM_IT_CC1 | TIM_IT_CC2));
  /* Enable the TIM1 Interrupt Request */
  //TIM_ITConfig(TIM1, (TIM_IT_Update | TIM_IT_CC1 | TIM_IT_CC2), ENABLE);
}

void init_tasks(void) {
  /* Initialize hardware */
  hardware_init();

  /* Create reset semaphore */
  xResetSemaphore = xSemaphoreCreateBinary();

  /* Create entre detector semaphore */
  xEntranceSemaphore = xSemaphoreCreateBinary();
  
  /* Create switch queue */
  xShootingQueue = xQueueCreate(10, (unsigned portBASE_TYPE)sizeof(IT_event));
  
  /* Create button queue */
  xButtonQueue = xQueueCreate(10, (unsigned portBASE_TYPE)sizeof(IT_event));
  
  
  xTaskCreate( vDragonTask, 
              "Dragon", 
              DRAGON_STACK_SIZE, 
              NULL, 
              DRAGON_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  /* Create switch queue */
  xSwitchQueue = xQueueCreate( SWITCH_NUMBER, 
                          ( unsigned portBASE_TYPE ) sizeof( signed char ) );
  
  /* Create painting task semaphore */
  xPaintingSemaphore = xSemaphoreCreateBinary();
  
  /* Create painting task */
  xTaskCreate( vPaintingTask, 
              "Painting", 
              PAINTING_STACK_SIZE, 
              NULL, 
              PAINTING_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
}

