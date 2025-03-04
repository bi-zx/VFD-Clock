#ifndef _key_driver_H
#define _key_driver_H

#include <OneButton.h>

#ifndef __key_driver_C
extern OneButton button1;
extern OneButton button2;
extern OneButton button3;
#endif

void key_init();
void key_tick(); // 新增按键检测函数

#endif
