#ifndef __PLAYER__
#define __PLAYER__

#define START_BYTE  (0x7E) 
#define VERSION     (0xFF)
#define PLAY_CMD    (0x03)
#define FEEDBACK    (0x01)
#define NO_FEEDBACK (0x00)
#define END_BYTE    (0xEF)


#define ENTRANCE_AUDIO        (1)
#define CORRECT_AUDIO         (2)
#define MINE_AUDIO            (3)
#define ORDER_ERR_AUDIO       (4)
#define TREAD_COMPLETE_AUDIO  (5)
#define LASER_CORRECT_AUDIO   (6)
#define LASER_ERR_AUDIO       (7)


int player_play_file(short index, TickType_t xTicksToWait);

#endif