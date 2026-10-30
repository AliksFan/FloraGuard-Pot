#include "UART.h"

/* ===== 接收环形缓冲 ===== */
static unsigned char idata rx_buf[UART1_BUF_LENGTH];
static volatile unsigned char rx_head = 0;
static volatile unsigned char rx_tail = 0;

/* 按旧项目公式：reload = 65536 - (FOSC/4)/BAUD */
static void UART1_SetBaud_T2(unsigned long FOSC, unsigned long BAUD)
{
    unsigned int reload = (unsigned int)(65536UL - (FOSC / 4UL) / BAUD);

    AUXR &= ~(1 << 4);   /* 停止 T2 */
    AUXR &= ~(1 << 3);   /* T2 作定时器 */
    AUXR |=  (1 << 2);   /* 1T 模式 */

    T2H = (unsigned char)(reload >> 8);
    T2L = (unsigned char)(reload & 0xFF);

    IE2  &= ~(1 << 2);   /* 禁止 T2 中断 */
    AUXR |=  (1 << 4);   /* 启动 T2 */
}

void UART1_Init_T2(unsigned long FOSC, unsigned long BAUD)
{
    /* UART1 波特率来源选择 T2，口映射 P3.0/P3.1 */
    AUXR |=  0x01;       /* S1_BRT=1 -> T2 供波特率 */
    P_SW1 &= ~(1 << 6);  /* UART1 on P3.0/P3.1 */
    P_SW1 &= ~(1 << 7);

    /* 串口模式1（8位可变波特率），允许接收 */
    SCON = (SCON & 0x3F) | 0x40;
    REN  = 1;

    UART1_SetBaud_T2(FOSC, BAUD);

    ES = 1;  /* 使能串口1中断（接收）*/
    TI = 1;  /* 允许首次发送 */
}

/* 发送 */
void UART1_WriteByte(unsigned char c)
{
    bit es_bak = ES;

    ES = 0;          /* Polling TX owns TI while sending. */
    SBUF = c;
    while (!TI);
    TI = 0;
    ES = es_bak;
}

void UART1_WriteString(const char *s)
{
    while (*s) {
        if (*s == '\n') UART1_WriteByte('\r');
        UART1_WriteByte(*s++);
    }
}

/* 接收 */
bit UART1_Available(void)
{
    return (rx_head != rx_tail);
}

unsigned char UART1_ReadByte(void)
{
    unsigned char c = 0;
    if (rx_head != rx_tail) {
        c = rx_buf[rx_tail++];
        if (rx_tail >= UART1_BUF_LENGTH) rx_tail = 0;
    }
    return c;
}

/* UART1 interrupt 4 */
void UART1_int(void) interrupt 4
{
    if (RI) {
        unsigned char next;
        RI = 0;
        next = rx_head + 1;
        if (next >= UART1_BUF_LENGTH) next = 0;

        if (next != rx_tail) {
            rx_buf[rx_head] = SBUF;
            rx_head = next;
        } else {
            (void)SBUF; /* 溢出丢字节 */
        }
    }
    if (TI) {
        TI = 0;
    }
}
