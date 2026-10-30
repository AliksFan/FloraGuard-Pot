#ifndef __ISR_H__
#define __ISR_H__

#include "STC8H.h"
#include "config.h"
#include "intrins.h"
#include "sleep.h"
#include "UART.h"
#include "rgb.h"

/* ================== 可按需修改 ================== */
#define LONG_PRESS_MS     (1200U)
#define DEBOUNCE_MS       (30U)             /* 去抖：30ms */

#define RED_BLINK_MS      (100U)            /* 充电中红灯快闪周期 */
#define GREEN_BLINK_MS    (100U)            /* 充满后绿灯快闪周期 */

/* 引脚：INT3-复位键，低电平按下；INT0-STD BY；INT1-CHRG */
sbit BTN_RST = P3^7;    /* INT3 */
sbit STDBY   = P3^2;    /* INT0: 充满时内部拉低 */
sbit CHRG    = P3^3;    /* INT1: 充电时内部拉低 */
/* =============================================== */

/* 初始化：配置 INT3/INT0/INT1（下降沿）、Timer1 1ms、总中断 */
void ISR_Init_All(void);

/* 软件复位（STC8H） */
void MCU_SoftwareReset(void);

/* —— 供项目自定义的事件钩子 —— */
void Hook_OnShortPress_Reset(void);        /* INT3 短按：复位 */
void Hook_OnLongPress_Sleep(void);         /* INT3 长按：睡眠 */

#endif /* __ISR_H__ */
