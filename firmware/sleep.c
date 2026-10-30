#include "STC8H.h"
#include "intrins.h"
#include "rgb.h"
#include "sleep.h"
#include "delay.h"
sbit SLEEP_BTN_RST = P3^7;
/* -------- 内部：一次进入掉电 -------- */
static void sleep_enter_once(void)
{
    _nop_();
    PCON |= 0x02;    /* PD=1 power down */
    _nop_();
    _nop_();

    if (SLEEP_BTN_RST == 0) {
        delay_ms(1200);
        if (SLEEP_BTN_RST == 0) {
            EA = 0;
            IAP_CONTR = 0x60;
            while (1);
        }
    }
}

/* -------- 内部：3 秒一次的掉电睡眠 --------
 * 说明：WKTCL 低 8 位，WKTCH 高 7 位+EN 位；
 *      3s ≈ 0.488ms * (N+1) → N≈6146≈0x1802
 *      你项目里沿用先前经验值的 0x97F3，保持一致性。
 */
static void sleep_3s_once(void)
{
    WKTCL = 0xF3;
    WKTCH = 0x97;   /* 高7位计数 + bit7=1(EN) */
    sleep_enter_once();
}

/* -------- 新增：100 ms 一次的掉电睡眠 ---------------
 * “睡眠一瞬”≈100ms → N ≈ 100ms / 0.488ms - 1 ≈ 204 = 0x00CC
 * 高 7 位为 0，WKTCH 只需置 EN 位
 */
static void sleep_100ms_once(void)  /* 睡眠一瞬（约100ms） */
{
    WKTCL = 0xCC;   /* 低8位 */
    WKTCH = 0x80;   /* EN=1，高7位=0 */
    sleep_enter_once();
}

/* 多个 3 秒片段累加 */
static void sleep_n3s(unsigned int n)
{
    while (n--)
        sleep_3s_once();
}

/* 12 小时（秒） */
// #define SEC_12H  (43200UL)
/* 1 分钟（秒） */
//#define SEC_12H  (60UL)
/* 3 小时（秒） */
 #define SEC_12H  (10800UL)

/* =========================================================
 * 永久掉电：外部中断/复位才能回来
 * =========================================================*/
void Sleep_Forever(void)
{
    for (;;)
    {
        /* 不需要定时唤醒，反复进入 PD 即可 */
        sleep_enter_once();
    }
}

/* =========================================================
 * 红蓝交替：
 *   改为：红亮 100ms → 灭 3s → 蓝亮 100ms → 灭 3s → 循环
 *   每轮约 6.2s（按 3s 灭灯计算循环次数：SEC_12H / 6）
 * =========================================================*/
void Sleep_Alt_RedBlue_3s_for_12h(void)
{
    unsigned long cycles;

    /* 两个“灭3s”构成一轮 → 6s 作为基准估算总轮数 */
    cycles = SEC_12H / 6UL;

    RGB_Off();
    while (cycles--)
    {
        RGB_Red();   sleep_100ms_once();  /* 亮 100ms */
        RGB_Off();   sleep_3s_once();     /* 灭 3s */
        RGB_Blue();  sleep_100ms_once();  /* 亮 100ms */
        RGB_Off();   sleep_3s_once();     /* 灭 3s */
    }
    RGB_Off();
}

/* =========================================================
 * 红灯：
 *   改为：亮 100ms → 灭 3s → 循环
 *   基于“灭3s”估算 → cycles = SEC_12H / 3
 * =========================================================*/
void Sleep_Red_3s_for_12h(void)
{
    unsigned long cycles;

    cycles = SEC_12H / 3UL;

    RGB_Off();
    while (cycles--)
    {
        RGB_Red();  sleep_100ms_once();   /* 亮 100ms */
        RGB_Off();  sleep_3s_once();      /* 灭 3s */
    }
    RGB_Off();
}

/* =========================================================
 * 绿灯：
 *   改为：亮 100ms → 灭 6s（= 3s×2）→ 循环
 *   基于“灭6s”估算 → cycles = SEC_12H / 6
 * =========================================================*/
void Sleep_Green_6s_for_12h(void)
{
    unsigned long cycles;

    cycles = SEC_12H / 6UL;

    RGB_Off();
    while (cycles--)
    {
        RGB_Green(); sleep_100ms_once();  /* 亮 100ms */
        RGB_Off();   sleep_n3s(2);        /* 灭 6s */
    }
    RGB_Off();
}

/* =========================================================
 * 蓝灯：
 *   改为：亮 100ms → 灭 3s → 循环
 *   基于“灭3s”估算 → cycles = SEC_12H / 3
 * =========================================================*/
void Sleep_Blue_3s_for_12h(void)
{
    unsigned long cycles;

    cycles = SEC_12H / 3UL;

    RGB_Off();
    while (cycles--)
    {
        RGB_Blue(); sleep_100ms_once();   /* 亮 100ms */
        RGB_Off();  sleep_3s_once();      /* 灭 3s */
    }
    RGB_Off();
}
