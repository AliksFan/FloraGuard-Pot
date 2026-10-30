#include "rgb.h"

/* 硬件位：红P1.3、绿P1.4、蓝P1.5 */
#define R_PIN 3
#define G_PIN 4
#define B_PIN 5

/* 1=共阳(低亮), 0=共阴(高亮) */
static unsigned char s_active_low = 0;

/* 只改动单个位（参数名用 pin，避免与 C51 关键字 bit 冲突） */
static void pin_write(unsigned char pin, unsigned char on)
{
    if (s_active_low) {                 /* 共阳：低=亮 */
        if (on)  P1 &= (unsigned char)~(1u << pin);
        else     P1 |=  (unsigned char) (1u << pin);
    } else {                            /* 共阴：高=亮 */
        if (on)  P1 |=  (unsigned char) (1u << pin);
        else     P1 &= (unsigned char)~(1u << pin);
    }
}

/* 同时设置三色（0/1） */
static void rgb_set(unsigned char r, unsigned char g, unsigned char b)
{
    pin_write(R_PIN, r);
    pin_write(G_PIN, g);
    pin_write(B_PIN, b);
}

void RGB_Init(unsigned char active_low)
{
    s_active_low = active_low ? 1 : 0;

#ifdef P1ASF
    /* 关闭 P1.3/1.4/1.5 模拟功能（若芯片带该寄存器） */
    P1ASF &= (unsigned char)~((1u << R_PIN) | (1u << G_PIN) | (1u << B_PIN));
#endif

    /* 三个引脚设为推挽输出：PxM1=0, PxM0=1 */
    P1M1 &= (unsigned char)~((1u << R_PIN) | (1u << G_PIN) | (1u << B_PIN));
    P1M0 |=  (unsigned char) ((1u << R_PIN) | (1u << G_PIN) | (1u << B_PIN));

    /* 上电默认全灭 */
    rgb_set(0,0,0);
}

/* 仅保留四个颜色方法 */
void RGB_Green(void)  { rgb_set(0,1,0); }
void RGB_Blue(void)   { rgb_set(0,0,1); }
void RGB_Yellow(void) { rgb_set(1,1,0); }  /* 黄=红+绿 */
void RGB_Red(void)    { rgb_set(1,0,0); }
void RGB_Off(void)    { rgb_set(0,0,0); }