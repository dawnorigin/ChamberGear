#ifndef __APPLICATIONCONFIG__
#define __APPLICATIONCONFIG__

#define KEY_JITTER_DELAY_MS     (15)
#define LAMP_ERROR_FLASH_MS     (200)
#define LAMP_ERROR_FLASH_TIMES  (5)
#define DJ_TREAD_DELAY_MS       (2000)
#define DJ_TREAD_RANDOM_TIMES   (6)
typedef struct {
  unsigned int event;
  TickType_t time_stamp;
} IT_event;

#endif
