#include "delay.h"

/* 这两个延时用于可见级别等待（心跳/模块关断），
   不要求精准；若要精准定时，请以后换成定时器。 */

void delay_10ms(unsigned int n)
{
    unsigned int i, j;
    while (n--)
    {
        for (i = 0; i < 100; ++i)
            for (j = 0; j < 120; ++j)
                ;
    }
}

void delay_ms(unsigned int ms)
{
    while (ms >= 10) { delay_10ms(1); ms -= 10; }
    if (ms) delay_10ms(1);
}
