#ifndef __APPLICATIONCONFIG__
#define __APPLICATIONCONFIG__

#define KEY_JITTER_DELAY_MS     (15)
#define ESCAPE_LASER_MS         (5000)
#define KEEP_LASER_ON_MS        (2000)
#define LED_BUTTON_ON_DELAY_MS  (3000)

#define BLOOD_TRUE_EYE          (5)
#define BLOOD_FALSE_EYE         (3)


#define SWITCH_NUMBER         (6)

typedef struct {
  unsigned int event;
  TickType_t time_stamp;
} IT_event;

#endif
