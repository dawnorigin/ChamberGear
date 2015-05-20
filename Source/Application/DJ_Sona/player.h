#ifndef __PLAYER__
#define __PLAYER__

#define START_BYTE  (0x7E) 
#define VERSION     (0xFF)
#define PLAY_CMD    (0x03)
#define FEEDBACK    (0x01)
#define NO_FEEDBACK (0x00)
#define END_BYTE    (0xEF)

#define FILE_1      (1)
#define FILE_2      (2)
#define FILE_3      (3)
#define FILE_4      (4)
#define FILE_5      (5)
#define FILE_6      (6)
#define FILE_7      (7)
#define FILE_8      (8)
#define FILE_9      (9)

#define ENTRANCE_AUDIO          FILE_1
#define CRAB_REMOVE_AUDIO       FILE_2
#define CRAB_SET_AUDIO          FILE_3
#define DJ_POWER_ON_AUDIO       FILE_4
#define DJ_BUTTON_ERR_AUDIO     FILE_5
#define DJ_TREAD_ERR_AUDIO      FILE_6
#define DOOR_OPEN_AUDIO         FILE_7

int player_play_file(short index, TickType_t xTicksToWait);

#endif