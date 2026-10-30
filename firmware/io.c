#include "io.h"

/* ------- 位定义（改为 P1.1） ------- */
#define PMOS_P1BIT  1   /* P1.1 PMOS：高=关，低=开 */

/* 电源状态缓存 */
static unsigned char s_power_on = 0;

/* ================= Init ================= */
void IO_Init(void)
{
#ifdef P1ASF
    /* 关闭 P1.1 的模拟功能（若有） */
    P1ASF &= (unsigned char)~(1u << PMOS_P1BIT);
#endif

    /* P1.1 推挽输出，默认高电平=关电源 */
    P1M1 &= (unsigned char)~(1u << PMOS_P1BIT);
    P1M0 |=  (unsigned char) (1u << PMOS_P1BIT);
    P1    |= (unsigned char) (1u << PMOS_P1BIT);

    s_power_on = 0;
}

/* ================= Power control (P1.1) ================= */
void IO_PowerOn(void)
{
    /* 低=开 */
    P1 &= (unsigned char)~(1u << PMOS_P1BIT);
    s_power_on = 1;
}

void IO_PowerOff(void)
{
    /* 高=关 */
    P1 |= (unsigned char) (1u << PMOS_P1BIT);
    s_power_on = 0;
}

unsigned char IO_PowerIsOn(void)
{
    return s_power_on;
}
