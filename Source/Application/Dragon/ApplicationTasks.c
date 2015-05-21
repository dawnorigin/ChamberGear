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
  SMOKE,
  IDLE,
  STEP1,
  STEP2,
  STEP3,
  STEP3_1,
  STEP3_2,
  STEP4,
  STEP4_1,
  STEP4_2,
  STEP5,
  STEP6,
  STEP7,
  ERR_ORDER,
  ERR_MINE,
  LASER,
  ERR_LASER,
  CRAB_BOX,
  ACTION,
  CAST,
  FINISH
};
/* Private macro -------------------------------------------------------------*/
#define JUMP_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)
#define JUMP_PRIORITY			  (tskIDLE_PRIORITY + 1 )

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
#define LASER_RX5_PIN         (GPIO_Pin_9)
#define LASER_RX_PIN_ALL      (LASER_RX1_PIN|LASER_RX2_PIN| \
                               LASER_RX3_PIN|LASER_RX4_PIN)
#define LASER_RX_STATUS(x)    GPIO_ReadInputDataBit(LASER_RX_PORT, (x))
#define ENABLE_LASER_REV_IT()   {EXTI->IMR |= LASER_REV_PIN;}
#define DISABLE_LASER_REV_IT()  {EXTI->IMR &= (~LASER_REV_PIN);}

#define HALL_PORT             (GPIOE)
#define HALL1_PIN             (GPIO_Pin_10)
#define HALL2_PIN             (GPIO_Pin_11)
#define HALL3_PIN             (GPIO_Pin_12)
#define HALL4_PIN             (GPIO_Pin_13)
#define HALL5_PIN             (GPIO_Pin_14)
#define HALL6_PIN             (GPIO_Pin_15)
#define HALL_PIN_ALL          (HALL1_PIN|HALL2_PIN|HALL3_PIN| \
                               HALL4_PIN|HALL5_PIN|HALL6_PIN)
#define HALL_STATUS(x)        GPIO_ReadInputDataBit(HALL_PORT, (x))
#define ENABLE_HALL_IT(x)     {EXTI->IMR |= (x);}
#define DISABLE_HALL_IT(x)    {EXTI->IMR &= (~(x));}

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
#define LASER_TX_OFF()        {GPIO_SetBits(LASER_TX_PORT, LASER_TX_PIN);}
#define LASER_TX_ON()         {GPIO_ResetBits(LASER_TX_PORT, LASER_TX_PIN);}

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
#define LED_F1_OFF(x)         {GPIO_SetBits(LED_F1_1_PIN, (x));}
#define LED_F1_ON(x)          {GPIO_ResetBits(LED_F1_1_PIN, (x));}

#define LED_F2_PORT           (GPIOD)
#define LED_F2_1_PIN          (GPIO_Pin_5)
#define LED_F2_2_PIN          (GPIO_Pin_6)
#define LED_F2_3_PIN          (GPIO_Pin_7)
#define LED_F2_PIN_ALL        (LED_F2_1_PIN|LED_F2_2_PIN|LED_F2_3_PIN)
#define LED_F2_OFF(x)         {GPIO_SetBits(LED_F2_1_PIN, (x));}
#define LED_F2_ON(x)          {GPIO_ResetBits(LED_F2_1_PIN, (x));}

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
#define LAMP_OFF(x)           {GPIO_SetBits(LAMP_PORT, LAMP_PIN);}
#define LAMP_ON(x)            {GPIO_ResetBits(LAMP_PORT, LAMP_PIN);}
/* Private variables ---------------------------------------------------------*/
SemaphoreHandle_t xResetSemaphore = NULL;
/* The queue used to hold step trod */
QueueHandle_t xTreadQueue;
SemaphoreHandle_t xLaserSemaphore = NULL;
SemaphoreHandle_t xCrabSemaphore = NULL;
SemaphoreHandle_t xPyroSemaphore = NULL;
/* Used for pyroelectric detector */
SemaphoreHandle_t xEntranceSemaphore = NULL;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static portTASK_FUNCTION( vJumpTask, pvParameters ) {
  int status = INIT;
  IT_event tread_event;
  TickType_t tick_tmp;
	/* The parameters are not used. */
	( void ) pvParameters;
  
  for(;;) {
    switch (status) {
      case INIT: {
        status = SMOKE;
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
  
  /* Configure input GPIO */
  GPIO_InitStructure.GPIO_Pin = PYRO1_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PYRO1_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = CRAB_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CRAB_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = LASER_REV_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
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
  
	EXTI_InitStructure.EXTI_Line = PYRO1_PIN|PYRO2_PIN;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);  
	
	EXTI_InitStructure.EXTI_Line = CRAB_PIN|TREAD_PIN_ALL|LASER_REV_PIN;
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

  /* Create entre detector semaphore */
  xEntranceSemaphore = xSemaphoreCreateBinary();
  
  /* Create switch queue */
  xTreadQueue = xQueueCreate(8, (unsigned portBASE_TYPE)sizeof(IT_event));  
  
  /* Create laser semaphore */
  xLaserSemaphore = xSemaphoreCreateBinary();
  
  /* Create crab remove semaphore */
  xCrabSemaphore = xSemaphoreCreateBinary();
  
  /* Create pyro detector semaphore */
  xPyroSemaphore = xSemaphoreCreateBinary();
  
  xTaskCreate( vJumpTask, 
              "Jump", 
              JUMP_STACK_SIZE, 
              NULL, 
              JUMP_PRIORITY, 
              ( TaskHandle_t * ) NULL );
  
}

