#ifndef __SLEEP_H__
#define __SLEEP_H__

#include "STC8H.h"

/* 永久掉电（外部中断或复位才会回来） */
void Sleep_Forever(void);

/* 低频闪烁模式（总时长约 12 小时）
 * - 红蓝交替，3s 分段：红亮3s→灭3s→蓝亮3s→灭3s→循环
 * - 红灯，3s 分段：亮3s→灭3s→循环
 * - 绿灯，6s 分段：亮6s→灭6s→循环
 * - 蓝灯，3s 分段：亮3s→灭3s→循环
 */
void Sleep_Alt_RedBlue_3s_for_12h(void);
void Sleep_Red_3s_for_12h(void);
void Sleep_Green_6s_for_12h(void);
void Sleep_Blue_3s_for_12h(void);

#endif /* __SLEEP_H__ */
