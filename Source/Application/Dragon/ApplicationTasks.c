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
  INIT_SHOOTING,
  SHOOTING,
  INIT_LASER,
  ESCAPE_LASER,
  INIT_BUTTON,
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
#define DRAGON_PRIORITY			  (tskIDLE_PRIORITY + 1)

#define BUTTON_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define BUTTON_PRIORITY			  (tskIDLE_PRIORITY + 2)

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
#define LASER_ESC_RX_STATUS()      GPIO_ReadInputDataBit(LASER_ESC_RX_PORT,\
                                    LASER_ESC_RX_PIN)

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

#define SWITCH_STATUS(x)      GPIO_ReadInputDataBit(HALL_PORT, \
                              ((HALL1_PIN >> 1) << (x)))
#define ENABLE_SWITCH_IT(x)   {EXTI->IMR |= ((HALL1_PIN >> 1) << (x));}

#define PLAYER_BUSY_PORT      (GPIOA)
#define PLAYER_BUSY_PIN       (GPIO_Pin_12)
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
#define LAMP_CAST_PIN         (GPIO_Pin_6)
#define LAMP_CAST_OFF()       {GPIO_SetBits(LAMP_CAST_PORT, LAMP_CAST_PIN);}
#define LAMP_CAST_ON()        {GPIO_ResetBits(LAMP_CAST_PORT, LAMP_CAST_PIN);}

#define BOX_CHEST_PORT        (GPIOE)
#define BOX_CHEST_PIN         (GPIO_Pin_4)
#define BOX_CHEST_OFF()       {GPIO_SetBits(BOX_CHEST_PORT, BOX_CHEST_PIN);}
#define BOX_CHEST_ON()        {GPIO_ResetBits(BOX_CHEST_PORT, BOX_CHEST_PIN);}

#define BOX_SWORD_PORT        (GPIOE)
#define BOX_SWORD_PIN         (GPIO_Pin_2)
#define BOX_SWORD_OFF()       {GPIO_SetBits(BOX_SWORD_PORT, BOX_SWORD_PIN);}
#define BOX_SWORD_ON()        {GPIO_ResetBits(BOX_SWORD_PORT, BOX_SWORD_PIN);}

#define DOOR_PORT             (GPIOE)
#define DOOR_PIN              (GPIO_Pin_0)
#define DOOR_OFF()            {GPIO_SetBits(DOOR_PORT, DOOR_PIN);}
#define DOOR_ON()             {GPIO_ResetBits(DOOR_PORT, DOOR_PIN);}

#define LAMP_PORT             (GPIOC)
#define LAMP_PIN              (GPIO_Pin_0)
#define LAMP_OFF()            {GPIO_SetBits(LAMP_PORT, LAMP_PIN);}
#define LAMP_ON()             {GPIO_ResetBits(LAMP_PORT, LAMP_PIN);}

#define RESERVED_PORT         (GPIOC)
#define RESERVED_PIN          (GPIO_Pin_2)
#define RESERVED_OFF()        {GPIO_SetBits(RESERVED_PORT, RESERVED_PIN);}
#define RESERVED_ON()         {GPIO_ResetBits(RESERVED_PORT, RESERVED_PIN);}
/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
/* Used for pyroelectric detector */
SemaphoreHandle_t xEntranceSemaphore = NULL;
/* Used for laser ray detector */
SemaphoreHandle_t xLaserRaySemaphore = NULL;
/* The queue used to hold shooting event */
QueueHandle_t xShootingQueue;
/* The queue used to hold buttons event */
QueueHandle_t xButtonQueue;
SemaphoreHandle_t xButtonSemaphore[4];
/* The queue used to hold hall event. */
QueueHandle_t xSwitchQueue;
/* Used for active painting task */
SemaphoreHandle_t xPaintingSemaphore = NULL;

uint8_t tasks[4] = {0, 1, 2, 3};

uint16_t led_button[4] = {LED_BUTTON1_PIN, LED_BUTTON2_PIN, 
                          LED_BUTTON3_PIN, LED_BUTTON4_PIN};
uint16_t button[4] = {BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN};
static int8_t blood[4] = {BLOOD_TRUE_EYE, BLOOD_TRUE_EYE, 
                          BLOOD_FALSE_EYE, BLOOD_FALSE_EYE};


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

int random_button(void) {
  int result;
  int blood_sum = blood[0] + blood[1] + blood[2] + blood[3];
  
  if (0 == blood_sum)
    return -1;
  
  result = rand() % blood_sum;
  
  if (blood[0]) {
    if (result < blood[0]) {
      return 0;
    }
  }
  if (blood[1]) {
    if (result < (blood[0] + blood[1])) {
      return 1;
    }
  }
  if (blood[2]) {
    if (result < (blood[0] + blood[1] + blood[2])) {
      return 2;
    }
  }
  return 3;
}

void led_button_reset(void) {
  blood[0] = blood[1] = BLOOD_TRUE_EYE; 
  blood[2] = blood[3] = BLOOD_FALSE_EYE;
}

void button_led_ctl(int index, int event) {
  uint16_t pins;
  
  if (index == event) {
    /* Play audio */
    player_play_file(BUTTON_CORRECT_AUDIO, 0);
    if (0 != blood[index])
      blood[index]--;
  } else if (BLOCK_LASER_EVENT == event) {
    /* Play audio */
    player_play_file(BLOCK_LASER_AUDIO, 0);
    /* Reset eyes' blood */
    led_button_reset();
  } else if (event < 2){
    /* Play audio */
    player_play_file(BUTTON_WRONG_AUDIO, 0);
    if (BLOOD_TRUE_EYE != blood[event])
      blood[event]++;
  } else {
    /* Play audio */
    player_play_file(BUTTON_WRONG_AUDIO, 0);
    if (BLOOD_FALSE_EYE != blood[event])
      blood[event]++;
  }
  pins = GPIO_Pin_All;
  switch (blood[0]) {
    case 5: pins &= ~LED_T1_5_PIN;
    case 4: pins &= ~LED_T1_4_PIN;
    case 3: pins &= ~LED_T1_3_PIN;
    case 2: pins &= ~LED_T1_2_PIN;
    case 1: pins &= ~LED_T1_1_PIN;
  }
  switch (blood[1]) {
    case 5: pins &= ~LED_T2_5_PIN;
    case 4: pins &= ~LED_T2_4_PIN;
    case 3: pins &= ~LED_T2_3_PIN;
    case 2: pins &= ~LED_T2_2_PIN;
    case 1: pins &= ~LED_T2_1_PIN;
  }
  switch (blood[2]) {
    case 3: pins &= ~LED_F1_3_PIN;
    case 2: pins &= ~LED_F1_2_PIN;
    case 1: pins &= ~LED_F1_1_PIN;
  }
  switch (blood[3]) {
    case 3: pins &= ~LED_F2_3_PIN;
    case 2: pins &= ~LED_F2_2_PIN;
    case 1: pins &= ~LED_F2_1_PIN;
  }
  GPIO_Write(LED_T_F_PORT, pins);
}
  
static portTASK_FUNCTION( vDragonTask, pvParameters ) {
  int status = INIT;
  IT_event event;
  uint16_t laser_rx_flag;
    
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
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
            status = INIT_SHOOTING;
          } else {
            ENABLE_PYRO_IT();
          }
        }
        break;
      }
      case INIT_SHOOTING: {
        /* Turn off the 6 laser transmitter */
        LASER_TX_OFF(LASER_TX_PIN_ALL);
        /* Clean shooting laser reciever interrupt */
        laser_rx_flag = 0;
        DISABLE_LASER_RX_IT(LASER_RX_PIN_ALL);
        while (pdTRUE == xQueueReceive(xShootingQueue, &event, 0));
        ENABLE_LASER_RX_IT(LASER_RX_PIN_ALL);
        /* Clean buttons interrupt */
        DISABLE_BUTTON_IT(BUTTON_PIN_ALL);
        while (pdTRUE == xQueueReceive(xButtonQueue, &event, 0));
        status = SHOOTING;
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
              status = INIT_LASER;
            }
          } else {
            ENABLE_LASER_RX_IT(LASER_RX_PIN_ALL);
          }
        }
        break;
      }
      case INIT_LASER: {
        vTaskDelay((TickType_t)3000);
        /* Play audio */
        player_play_file(LASER_AUDIO, 0);
        /* Wait for audio starting */
        while(Bit_RESET != PLAYER_BUSY_STATUS());
        /* Clean laser ray detector semaphore */
        while (pdTRUE == xSemaphoreTake(xLaserRaySemaphore, 0));
        /* Wait for audio end */
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        /* Clear TIM1 interrupt pending bit */
        TIM_ClearITPendingBit(TIM1, (TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3));
        /* Enable the TIM1 Interrupt Request */
        TIM_ITConfig(TIM1, (TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3), ENABLE);
        /* Clear TIM1 counter */
        TIM_SetCounter(TIM1, 0);
        /* TIM enable counter */
        TIM_Cmd(TIM1, ENABLE);
        /* Turn on the 6 laser transmitter */
        LASER_TX_ON(LASER_TX_PIN_ALL);
        status = ESCAPE_LASER;
        break;
      }
      case ESCAPE_LASER: {
        if (pdTRUE == xSemaphoreTake(xLaserRaySemaphore, ESCAPE_LASER_MS)) {
          /* The laser ray has been released */
          status = INIT_BUTTON;
        } else {
          player_play_file(RESHOOTING_AUDIO, 0);
          /* The laser ray has been blocked */
          status = INIT_SHOOTING;
        }
        TIM_ITConfig(TIM1, (TIM_IT_CC1 | TIM_IT_CC3), DISABLE);
        break;
      }
      case INIT_BUTTON: {
        /* Play audio */
        player_play_file(BUTTON_AUDIO, 0);
        /* Wait for audio starting */
        while(Bit_RESET != PLAYER_BUSY_STATUS());
        /* Wait for audio end */
        for (;;) {
          if (Bit_RESET != PLAYER_BUSY_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        /* LEDs show */
        LED_BUTTON_ON(LED_BUTTON_PIN_ALL);
        LED_T_F_ON(LED_T_F_PIN_ALL);
        vTaskDelay((TickType_t)1000);
        LED_BUTTON_OFF(LED_BUTTON_PIN_ALL);
        /* Reset eyes' blood */
        led_button_reset();
        while (xQueueReceive(xButtonQueue, &event, 0));
        /* Init random seed */
        srand(xTaskGetTickCount());
        status = BUTTON;
        break;
      }
      case BUTTON: {
        /* Wait for laser on */
        for (;;) {
          if (Bit_RESET != LASER_ESC_RX_STATUS())
            break;
          vTaskDelay((TickType_t)10);
        }
        int index;
        /* A random button */
        index = random_button();
        /* Enable button interrupt */
        ENABLE_BUTTON_IT(BUTTON_PIN_ALL);
        /* Check whether the button is vailed */
        if (index != -1) {
          /* At least one eye has blood */
          LED_BUTTON_ON(led_button[index]);
          if (xQueueReceive(xButtonQueue, &event, LED_BUTTON_ON_DELAY_MS)) {
            button_led_ctl(index, event.event);
            while (xQueueReceive(xButtonQueue, &event, 0)) {
              button_led_ctl(index, event.event);
            }
          }
          LED_BUTTON_OFF(led_button[index]);
          while (xQueueReceive(xButtonQueue, &event, LED_BUTTON_OFF_DELAY_MS)) {
            button_led_ctl(-1, event.event);
          }
        } else {
          /* All eyes has no blood */
          TIM_Cmd(TIM1, DISABLE);
          TIM_ITConfig(TIM1, (TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3), DISABLE);
          LED_BUTTON_OFF(BUTTON_PIN_ALL);
          LASER_TX_OFF(LASER_TX_PIN_ALL);
          status = INIT_PAINTING;
          vTaskDelay((TickType_t)3000);
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
        ENABLE_HALL_IT(HALL_PIN_ALL);
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
                ENABLE_HALL_IT(HALL_PIN_ALL);
              }
            } else {
              /* The passing switch is inactive */
              status = INIT;
              ENABLE_HALL_IT(HALL_PIN_ALL);
            }
          } else {
            /* Enable interrupt */
            ENABLE_HALL_IT(HALL_PIN_ALL);
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

static portTASK_FUNCTION(vButtonTask, pvParameters) {
  int index = *((char*)pvParameters);
  IT_event event;
  
  event.event = index;
  for(;;) {
    if (pdTRUE == xSemaphoreTake(xButtonSemaphore[index], portMAX_DELAY)) {
      /* Skip the key jitter step */
      vTaskDelay((TickType_t)KEY_JITTER_DELAY_MS);
      if (Bit_RESET == BUTTON_STATUS(button[index])) {
        xQueueSend(xButtonQueue, &event, 0);
      }
      ENABLE_BUTTON_IT(button[index]);
    }
  }//for(;;)
}

static void hardware_init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_ICInitTypeDef TIM_ICInitStructure;
  TIM_OCInitTypeDef TIM_OCInitStructure;
  
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
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = (36000 - 1);
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
  /* Select the TIM1 Input Trigger: TI1FP1 */
  TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1);
  /* Select the slave Mode: Reset Mode */
  TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);
  /* Enable the Master/Slave Mode */
  TIM_SelectMasterSlaveMode(TIM1, TIM_MasterSlaveMode_Enable);
  
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0x0;
  TIM_PWMIConfig(TIM1, &TIM_ICInitStructure);

  /* Output Compare Active Mode configuration: Channel3 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Active;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
  TIM_OCInitStructure.TIM_Pulse = (KEEP_LASER_ON_MS * 2);
  TIM_OC3Init(TIM1, &TIM_OCInitStructure);
}

void init_tasks(void) {
  char i;
  char task_name[configMAX_TASK_NAME_LEN] = {'B','u','t','t','o','n','1',0};
  
  /* Initialize hardware */
  hardware_init();

  /* Create reset semaphore */
  xResetSemaphore = xSemaphoreCreateBinary();

  /* Create entre detector semaphore */
  xEntranceSemaphore = xSemaphoreCreateBinary();
  
  /* Create switch queue */
  xShootingQueue = xQueueCreate(10, (unsigned portBASE_TYPE)sizeof(IT_event));
  
  /* Create laser ray detector semaphore */
  xLaserRaySemaphore = xSemaphoreCreateBinary();
  
  /* Create button queue */
  xButtonQueue = xQueueCreate(10, (unsigned portBASE_TYPE)sizeof(IT_event));
  
  
  xTaskCreate( vDragonTask, 
              "Dragon", 
              DRAGON_STACK_SIZE, 
              NULL, 
              DRAGON_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
  for (i = 0; i < 4; i++) {
    xButtonSemaphore[i] = xSemaphoreCreateBinary();
    task_name[6] = i + '1';
    xTaskCreate(vButtonTask,
                task_name,
                BUTTON_STACK_SIZE, 
                (tasks + i),
                BUTTON_PRIORITY,
                (TaskHandle_t*) NULL );
  }
  
  /* Create hall queue */
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

