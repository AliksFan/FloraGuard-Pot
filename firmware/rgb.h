#ifndef __RGB_H__
#define __RGB_H__

#include "STC8H.h"

/* 初始化：
 *   active_low = 1  → 共阳（低电平点亮）
 *   active_low = 0  → 共阴（高电平点亮）
 */
void RGB_Init(unsigned char active_low);

/* 仅保留这四个方法 */
void RGB_Green(void);
void RGB_Blue(void);
void RGB_Yellow(void);   /* 黄=红+绿 */
void RGB_Red(void);
void RGB_Off(void);

#endif
