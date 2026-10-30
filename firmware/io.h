#ifndef __IO_H__
#define __IO_H__

#include "STC8H.h"

/* ---------------------------------------------------------
 * IO 电源控制（P1.1 PMOS 开关）
 *   - IO_Init()      : 初始化 P1.1 为推挽输出，默认高电平=断电
 *   - IO_PowerOn()   : 拉低 P1.1 → 打开外设供电
 *   - IO_PowerOff()  : 拉高 P1.1 → 关闭外设供电
 *   - IO_PowerIsOn() : 返回当前电源开关状态（软件缓存）
 * --------------------------------------------------------- */

void IO_Init(void);
void IO_PowerOn(void);
void IO_PowerOff(void);
unsigned char IO_PowerIsOn(void);

#endif /* __IO_H__ */
