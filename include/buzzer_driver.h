#ifndef _buzzer_driver_H
#define _buzzer_driver_H

#include "freertos/queue.h"

extern QueueHandle_t bz_queue;

#define DDIDI   1//滴~滴 音效
#define DIDI    2//滴 滴 音效
#define DI      3//滴 音效
#define CONDI   4//持续 滴~
#define EXITDI  5//关闭持续 滴~

void buzzer_init();

#endif
