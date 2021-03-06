#ifndef __PLAYER__
#define __PLAYER__

#define START_BYTE  (0x7E) 
#define VERSION     (0xFF)
#define PLAY_CMD    (0x03)
#define FEEDBACK    (0x01)
#define NO_FEEDBACK (0x00)
#define END_BYTE    (0xEF)

#define ENTRANCE_AUDIO        (1)
#define SHOOTING_AUDIO        (2)
#define LASER_AUDIO           (3)
#define RESHOOTING_AUDIO      (4)
#define BUTTON_AUDIO          (5)
#define BUTTON_CORRECT_AUDIO  (6)
#define BUTTON_WRONG_AUDIO    (7)
#define BLOCK_LASER_AUDIO     (8)
#define SWORD_AUDIO           (9)
#define OPEN_DOOR_AUDIO       (10)

int player_play_file(short index, TickType_t xTicksToWait);

#endif