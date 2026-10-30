#ifndef __CONFIG_H__
#define __CONFIG_H__

/* === 时钟与串口参数 ===
 * 你已验证内部 IRC ≈ 11.0592MHz
 */
#define FOSC_HZ    11059200UL   /* 实测可用 */
#define BAUD_RATE  9600UL       /* 稳定优先；确认OK可改 115200UL */

/* 1=temporary battery calibration firmware, 0=normal plant-guard firmware */
#define BATTERY_CALIBRATION_MODE  0

#endif
