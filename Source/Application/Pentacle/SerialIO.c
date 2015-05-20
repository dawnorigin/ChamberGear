/* Includes ------------------------------------------------------------------*/
/* Library includes. */
#include "stm32f10x.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "SerialIO.h"


static unsigned char buffer_tx[USART_TX_BUFFER_LENGTH];
static unsigned char buffer_rx[USART_RX_BUFFER_LENGTH];

volatile BufferType USART1_tx_buf = {.buffer = buffer_tx, .head = 0, .tail = 0};
volatile BufferType USART1_rx_buf = {.buffer = buffer_rx, .head = 0, .tail = 0};

void USART1_send_string(char* str) {
  while (*str) {
    USART1_tx_buf.buffer[USART1_tx_buf.head++] = *str++;
    if (USART_TX_BUFFER_LENGTH == USART1_tx_buf.head)
      USART1_tx_buf.head = 0;
  }
  USART_ITConfig( USART1, USART_IT_TXE, ENABLE );
}

void USART1_put(char* str, int num) {
  while (num--) {
    USART1_tx_buf.buffer[USART1_tx_buf.head++] = *str++;
    if (USART_TX_BUFFER_LENGTH == USART1_tx_buf.head)
      USART1_tx_buf.head = 0;
  }
  USART_ITConfig( USART1, USART_IT_TXE, ENABLE );
}

char USART1_recieve_char(void) {
  return USART_ReceiveData(USART1);
}




