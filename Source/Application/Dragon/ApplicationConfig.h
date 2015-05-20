#ifndef __APPLICATIONCONFIG__
#define __APPLICATIONCONFIG__

#define KEY_JITTER_DELAY_MS   (15)
#define TWO_FEET_INTERVAL_MS  (500)
#define ESCAPE_LASER_MS       (3000)
#define KEEP_LASER_ON_MS      (500)
#define LAMP_ERR_FLASH_MS     (300)

typedef struct {
  unsigned int event;
  TickType_t time_stamp;
} IT_event;

#endif
