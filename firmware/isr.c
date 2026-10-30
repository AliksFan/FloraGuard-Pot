#include "isr.h"

/* ============ 充电状态机 ============ */
typedef enum {
    MODE_NORMAL = 0,        /* 正常运行，允许 INT3 键逻辑 */
    MODE_CHARGING,          /* 充电中：红灯快闪 */
    MODE_CHARGED            /* 已充满：绿灯快闪 */
} charge_mode_t;

static volatile charge_mode_t g_mode = MODE_NORMAL;

/* ============ INT3（复位键）内部状态 ============ */
static volatile bit g_measuring = 0;          /* 是否正在计时（已按下） */
static volatile unsigned int g_ms = 0;        /* 已累计毫秒 */
static volatile unsigned int g_debounce = 0;  /* 去抖计数 */

/* ============ 快闪控制（由 T2 1ms 驱动） ============ */
static volatile unsigned int g_blink_ms = 0;
static volatile bit g_blink_on = 0;
static unsigned int g_t1_reload = 0;
/* ============ 工具函数 ============ */
void MCU_SoftwareReset(void)
{
    EA = 0;
    IAP_CONTR = 0x60;       /* 软件复位（STC8H) */
    while (1);
}

void Hook_OnShortPress_Reset(void)
{
    /* Short press ignored; hold P3.7 to reset. */
}

void Hook_OnLongPress_Sleep(void)
{
    RGB_Off();
    MCU_SoftwareReset();
}

/* 进入充电模式：红灯快闪，屏蔽 INT3 键逻辑 */
static void EnterChargingMode(void)
{
    g_mode = MODE_CHARGING;
    g_blink_ms = 0;
    g_blink_on = 0;
    RGB_Off();
    UART1_WriteString("[CHRG] charging detected -> RED fast blink\r\n");
}

/* 进入充满模式：绿灯快闪，屏蔽 INT3 键逻辑 */
static void EnterChargedMode(void)
{
    g_mode = MODE_CHARGED;
    g_blink_ms = 0;
    g_blink_on = 0;
    RGB_Off();
    UART1_WriteString("[STDBY] charge complete -> GREEN fast blink\r\n");
}

/* 按当前模式执行一次闪烁输出 */
static void BlinkStep(void)
{
    if (g_mode == MODE_CHARGING)
    {
        if (g_blink_on) RGB_Red(); else RGB_Off();
    }
    else if (g_mode == MODE_CHARGED)
    {
        if (g_blink_on) RGB_Green(); else RGB_Off();
    }
}

/* ============ Timer2：1ms 节拍 ============ */
/* 定时器时钟 = Fosc/12；1ms 重装值 = 65536 - Fosc/12/1000 */
static void T1_Init_1ms(void)
{
    unsigned long tck = FOSC_HZ / 12UL;
    g_t1_reload = 65536U - (unsigned int)(tck / 1000UL);

    TR1 = 0;
    TMOD = (TMOD & 0x0F) | 0x10;       /* Timer1 mode 1, 16-bit */
    AUXR &= (unsigned char)~0x40;      /* Timer1 Fosc/12 */
    TH1 = (unsigned char)(g_t1_reload >> 8);
    TL1 = (unsigned char)g_t1_reload;
    ET1 = 1;
    TR1 = 1;
}

/* ============ INT3（下降沿） ============ */
static void INT3_Init_FallingEdge(void)
{
    /* P3.7：准双向（上拉），空闲为高；按键接地为低 */
    P3M0 &= ~(1 << 7);
    P3M1 &= ~(1 << 7);

    IE2 |= (1 << 3);        /* EX3 = 1 使能 INT3 */
}

/* ============ INT0/INT1（下降沿） ============ */
static void INT01_Init_FallingEdge(void)
{
    /* P3.2 / P3.3 设为准双向输入，上拉保证空闲为高电平 */
    P3M0 &= ~((1 << 2) | (1 << 3));
    P3M1 &= ~((1 << 2) | (1 << 3));

    /* 选择边沿触发：IT0=1、IT1=1（TCON） */
    IT0 = 1;
    IT1 = 1;

    /* 允许外部中断 0/1 */
    EX0 = 1;
    EX1 = 1;
}

/* 采样上电时充电状态（避免错过初始状态） */
static void SampleChargePinsOnBoot(void)
{
    if (CHRG == 0)       /* 低=正在充电 */
    {
        EnterChargingMode();
    }
    else if (STDBY == 0) /* 低=充满 */
    {
        EnterChargedMode();
    }
    else
    {
        g_mode = MODE_NORMAL;
        RGB_Off();
    }
}

/* ============ 对外初始化 ============ */
void ISR_Init_All(void)
{
    RGB_Off();                  /* 先关灯，避免上电误亮 */
    INT3_Init_FallingEdge();    /* 复位键 */
    INT01_Init_FallingEdge();   /* 充电/充满指示 */
    T1_Init_1ms();

    EA = 1;
    UART1_WriteString("[ISR] INT3/INT0/INT1 + Timer1(1ms) initialized\r\n");

    /* 根据当前引脚状态设置模式（避免上电已在充电中） */
    SampleChargePinsOnBoot();
}

/* =========================================================
 *                    中断服务程序
 * ========================================================= */

/* Timer2 中断向量：interrupt 12（STC8H） */
void TMR1_ISR(void) interrupt 3
{
    TH1 = (unsigned char)(g_t1_reload >> 8);
    TL1 = (unsigned char)g_t1_reload;

    /* —— 充电状态下：驱动快闪 —— */
    if (g_mode == MODE_CHARGING || g_mode == MODE_CHARGED)
    {
        unsigned int period = (g_mode == MODE_CHARGING) ? RED_BLINK_MS : GREEN_BLINK_MS;

        if (++g_blink_ms >= period)
        {
            g_blink_ms = 0;
            g_blink_on = !g_blink_on;
            BlinkStep();
        }
        /* 屏蔽 INT3 的长按短按流程 */
        return;
    }

    /* —— 正常模式下：处理 INT3 去抖与长按 —— */
    if (g_measuring)
    {
        if (g_debounce < DEBOUNCE_MS)
        {
            g_debounce++;
        }
        else
        {
            g_ms++;
            if (g_ms >= LONG_PRESS_MS)
            {
                g_measuring = 0;
                g_ms = 0;
                g_debounce = 0;
                Hook_OnLongPress_Sleep();
            }
        }

        /* 监测松开（空闲为高电平） */
        if (BTN_RST)
        {
            if (g_debounce >= DEBOUNCE_MS && g_ms < LONG_PRESS_MS)
            {
                g_measuring = 0;
                g_debounce = 0;
                g_ms = 0;
                Hook_OnShortPress_Reset();
            }
            else
            {
                g_measuring = 0;
                g_debounce = 0;
                g_ms = 0;
            }
        }
    }
}

/* INT3 中断向量：interrupt 11（下降沿，仅在正常模式下响应） */
void INT3_ISR(void) interrupt 11
{
    g_measuring = 1;
    g_ms = 0;
    g_debounce = 0;
}

/* INT0（STDBY）：充电完成时内部拉低 -> 绿灯快闪 */
void INT0_ISR(void) interrupt 0
{
    /* 下降沿进入“充满”模式 */
    EnterChargedMode();
}

/* INT1（CHRG）：充电时内部拉低 -> 红灯快闪 */
void INT1_ISR(void) interrupt 2
{
    /* 下降沿进入“充电中”模式 */
    EnterChargingMode();
}

