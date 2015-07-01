#ifndef __PLAYER__
#define __PLAYER__

#define START_BYTE  (0x7E) 
#define VERSION     (0xFF)
#define PLAY_CMD    (0x03)
#define FEEDBACK    (0x01)
#define NO_FEEDBACK (0x00)
#define END_BYTE    (0xEF)

#define ENTRANCE_AUDIO          (1)
#define CRAB_REMOVE_AUDIO       (2)
#define CRAB_SET_AUDIO          (3)
#define DJ_POWER_ON_AUDIO       (4)
#define DJ_BUTTON_ERR_AUDIO     (5)
#define DJ_BLUE_AUDIO           (6)
#define DJ_GREEN_AUDIO          (7)
#define DJ_PINK_AUDIO           (8)
#define DJ_TREAD_CORRECT_AUDIO1 (9)
#define DJ_TREAD_CORRECT_AUDIO2 (10)
#define DJ_TREAD_CORRECT_AUDIO3 (11)
#define DJ_TREAD_CORRECT_AUDIO4 (12)
#define DJ_TREAD_CORRECT_AUDIO5 (13)
#define DJ_TREAD_CORRECT_AUDIO6 (14)
#define DJ_TREAD_ERR_AUDIO      (15)
#define DJ_TREAD_COMPLETE_AUDIO (16)
#define DOOR_OPEN_AUDIO         (17)

int player_play_file(short index, TickType_t xTicksToWait);

#endif