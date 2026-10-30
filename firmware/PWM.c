#include "STC8H.h"
#include "config.h"
#include "PWM.h"

/* ========= 扩展 SFR 开关 ========= */
static unsigned char enter_eaxfr(void)
{
    unsigned char b = P_SW2;
    P_SW2 = b | 0x80;     /* EAXFR = 1 */
    return b;
}
static void leave_eaxfr(unsigned char b)
{
    P_SW2 = b;
}

/* ========= P5.4：推挽输出、关模拟 ========= */
static void p54_as_pp_out(void)
{
#ifdef P5ASF
    P5ASF &= ~(1 << 4);
#endif
    P5M1 &= ~(1 << 4);
    P5M0 |=  (1 << 4);
}

/* ========= GPIO<->PWM 切换 ========= */
static void p54_to_gpio_low(void)
{
#ifdef P5ASF
    P5ASF &= ~(1 << 4);
#endif
    P5M1 &= ~(1 << 4);
    P5M0 |=  (1 << 4);
    P5   &= ~(1 << 4);    /* 输出低 */
}

static void p54_to_pwm_func(void)
{
#ifdef P5ASF
    P5ASF &= ~(1 << 4);
#endif
    P5M1 &= ~(1 << 4);
    P5M0 |=  (1 << 4);
    /* 不主动写 P5 位，交由外设驱动 */
}

/* ========= 频率配置 =========
 * Fpwm = Fosc / ((PSC+1)*(ARR+1))；此处固定 PSC=0 → /1
 */
static void pwmb_set_freq(unsigned long freq_hz)
{
    unsigned long arrp1;
    unsigned char bk;

    if (freq_hz == 0) freq_hz = 1;
    arrp1 = FOSC_HZ / freq_hz;
    if (arrp1 == 0)        arrp1 = 1;
    if (arrp1 > 65536UL)   arrp1 = 65536UL;

    bk = enter_eaxfr();
    PWMB_PSCRH = 0x00;                           /* PSC = 0 */
    PWMB_PSCRL = 0x00;
    PWMB_ARRH  = (unsigned char)(((arrp1 - 1) >> 8) & 0xFF);
    PWMB_ARRL  = (unsigned char)((arrp1 - 1) & 0xFF);
    PWMB_EGR  |= 0x01;                           /* UG=1，更新预装载 */
    leave_eaxfr(bk);
}

/* ========= 占空比配置：PWM6(通道2) ========= */
static void pwmb_set_duty_percent(unsigned int duty_percent)
{
    unsigned int  arr;
    unsigned long arrp1;
    unsigned long ccr;
    unsigned char bk;

    if (duty_percent > 100) duty_percent = 100;

    bk  = enter_eaxfr();
    arr = (((unsigned int)PWMB_ARRH) << 8) | PWMB_ARRL;
    leave_eaxfr(bk);

    arrp1 = (unsigned long)arr + 1UL;
    ccr   = (arrp1 * (unsigned long)duty_percent) / 100UL;
    if (ccr) ccr--;                                  /* 防 100% 越界 */

    bk = enter_eaxfr();
    PWMB_CCR6H = (unsigned char)((ccr >> 8) & 0xFF);
    PWMB_CCR6L = (unsigned char)(ccr & 0xFF);
    PWMB_EGR  |= 0x01;                               /* UG=1 生效 */
    leave_eaxfr(bk);
}

/* ========= 初始化：PWM6 → P5.4 ========= */
void PWM_Init(unsigned long freq_hz)
{
    unsigned char bk;

    p54_as_pp_out();          /* 保证脚为推挽可驱动 */

    bk = enter_eaxfr();

    /* --- 映射：PWM6 → P5.4 ---
     * C6PS 位段在 PWMB_PS 的 [5:4]；01b 选择 P5.4
     * 掩码 0x30，赋值 0x10
     */
    PWMB_PS = (PWMB_PS & (unsigned char)~0x30) | 0x10;

    /* 停机 + 预装载 */
    PWMB_CR1  &= (unsigned char)~0x01;   /* CEN=0 */
    PWMB_CR1  |= 0x80;                   /* ARPE=1 */

    /* 通道2：PWM 模式1 + 预装载 */
    PWMB_CCMR2 &= (unsigned char)~0x70;  /* 清 OC2M */
    PWMB_CCMR2 |= 0x60;                  /* OC2M=110 (PWM1) */
    PWMB_CCMR2 |= 0x08;                  /* OC2PE=1 */

    PWMB_CCER1 |= 0x10;                  /* CC2E=1 允许输出 */
    PWMB_CCER1 &= (unsigned char)~0x20;  /* CC2P=0 不反相 */
    PWMB_BKR   |= 0x80;                  /* MOE=1 主输出使能 */

    /* 使能 PWM6 的 ENO 位（bit2 = 0x04） */
    PWMB_ENO   |= 0x04;

    leave_eaxfr(bk);

    /* 频率 & 默认占空比 50% */
    pwmb_set_freq(freq_hz);
    pwmb_set_duty_percent(50);
}

/* ========= 设占空比 ========= */
void PWM_SetDuty(unsigned char ch, unsigned int duty_percent)
{
    (void)ch;  /* 固定 PWM6 */
    pwmb_set_duty_percent(duty_percent);
}

/* ========= 启动：无毛刺切换到 PWM 驱动 ========= */
void PWM_Start(unsigned char ch)
{
    unsigned char bk;
    (void)ch;  /* 固定 PWM6 @ P5.4 */

    /* 先把脚保持为 GPIO 低，防止接管瞬间毛刺 */
    p54_to_gpio_low();

    bk = enter_eaxfr();

    /* 再次确保映射：PWM6 → P5.4（[5:4]=01） */
    PWMB_PS  = (PWMB_PS & (unsigned char)~0x30) | 0x10;

    /* 允许 PWM6 输出（bit2） */
    PWMB_ENO |= 0x04;

    /* 启动计数器 */
    PWMB_CR1 |= 0x01;   /* CEN=1 */

    leave_eaxfr(bk);

    /* 交由外设驱动该脚 */
    p54_to_pwm_func();
}

/* ========= 停止：释放外设并强制低电平 ========= */
void PWM_Stop(unsigned char ch)
{
    unsigned char bk;
    (void)ch;  /* 固定 PWM6 @ P5.4 */

    bk = enter_eaxfr();

    /* 停止计数器、关闭该通道输出使能（bit2） */
    PWMB_CR1 &= (unsigned char)~0x01;   /* CEN=0 */
    PWMB_ENO &= (unsigned char)~0x04;   /* 禁止 PWM6 输出 */

    leave_eaxfr(bk);

    /* 切换为 GPIO 低电平，保证停在低 */
    p54_to_gpio_low();
}
