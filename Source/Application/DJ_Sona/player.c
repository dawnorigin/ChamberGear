/* Includes ------------------------------------------------------------------*/
/* Library includes. */
#include "stm32f10x.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "SerialIO.h"
#include "player.h"



static char player_cmd[] = {0x7E,0xFF,0x06,0x03,0x01,0,0,0,0,0xEF};
static char player_stop_cmd[] = {0x7E,0xFF,0x06,0x16,0,0,0,0xFE,0xE5,0xEF};

void player_stop(void){
  short checksum = VERSION + 0x16 + 0x06;
  checksum = 0 - checksum;
  player_cmd[7] = (unsigned char)(checksum >> 8);
  player_cmd[8] = (unsigned char)checksum;
  USART1_put(player_stop_cmd, 10);
}

int player_play_file(short index, TickType_t xTicksToWait) {
  short checksum = VERSION + PLAY_CMD + FEEDBACK + 0x06;
  player_cmd[5] = (unsigned char)(index >> 8);
  player_cmd[6] = (unsigned char)index;
  checksum = 0 - (checksum + player_cmd[5] + player_cmd[6]);
  player_cmd[7] = (unsigned char)(checksum >> 8);
  player_cmd[8] = (unsigned char)checksum;
  USART1_put(player_cmd, 10);
  return 0;
}