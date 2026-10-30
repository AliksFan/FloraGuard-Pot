#ifndef __UART_H__
#define __UART_H__

#include "STC8H.h"

/* 接收环形缓冲长度 */
#define UART1_BUF_LENGTH  64

/* UART1 初始化：使用 Timer2 作为波特率源（与你旧项目一致：FOSC/4/BAUD） */
void UART1_Init_T2(unsigned long FOSC, unsigned long BAUD);

/* 发送 */
void UART1_WriteByte(unsigned char c);
void UART1_WriteString(const char *s);

/* 接收（非阻塞） */
bit            UART1_Available(void);
unsigned char  UART1_ReadByte(void);

#endif
