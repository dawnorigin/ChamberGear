#ifndef __PLAYER__
#define __PLAYER__

#define START_BYTE  (0x7E) 
#define VERSION     (0xFF)
#define PLAY_CMD    (0x03)
#define FEEDBACK    (0x01)
#define NO_FEEDBACK (0x00)
#define END_BYTE    (0xEF)

#define CORRECT_AUDIO       (1)
#define ERROR_AUDIO         (2)
#define COMPLETE_AUDIO      (3)

int player_play_file(short index, TickType_t xTicksToWait);

#endif