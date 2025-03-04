#ifndef _clock_function_H
#define _clock_function_H
#include "freertos/event_groups.h"

#ifndef __clock_function_C

extern EventGroupHandle_t KeyEventHandle; //按键事件消息队列句柄
#endif // !__ClockFunction_C
void clock_funtion_task();
void clock_funtion_event_task(void* ary);
#endif
