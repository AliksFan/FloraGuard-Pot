#ifndef __DELAY_H__
#define __DELAY_H__

/* 简单延时：10ms 粗粒度（与 FOSC 大致相关，但足够做心跳/等待） */
void delay_10ms(unsigned int n);

/* 方便：毫秒延时（粗略） */
void delay_ms(unsigned int ms);

#endif
