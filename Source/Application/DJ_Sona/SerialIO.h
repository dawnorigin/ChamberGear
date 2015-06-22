#ifndef __SERIALIO__
#define __SERIALIO__

#define USART_TX_BUFFER_LENGTH 250
#define USART_RX_BUFFER_LENGTH 60


typedef struct {
  unsigned char * buffer;
  unsigned char head;
  unsigned char tail;
}BufferType, *BufferTypePtr;

extern volatile BufferType USART1_tx_buf;
extern volatile BufferType USART1_rx_buf;

void USART1_send_string(char* str);
void USART1_put(char* str, int num);

#endif