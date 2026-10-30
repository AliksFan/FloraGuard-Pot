#ifndef __PWM_H__
#define __PWM_H__

#include "STC8H.h"
#include "config.h"

void PWM_Init(unsigned long freq_hz);
/* 占空比百分比 0..100（ch 传 1 即可，内部固定 PWM5） */
void PWM_SetDuty(unsigned char ch, unsigned int duty_percent);
/* 启停（ch 传 1 即可） */
void PWM_Start(unsigned char ch);
void PWM_Stop (unsigned char ch);

#endif
