#include "adc.h"
#include <intrins.h>   /* for _nop_() */

/* -------- ADC_CONTR 位定义（STC8H 常见） --------
 * bit7: ADC_POWER
 * bit6: ADC_START
 * bit5: ADC_FLAG (EOC)
 * bit4..0: 通道号
 */
#define ADC_POWER  0x80
#define ADC_START  0x40
#define ADC_FLAG   0x20
#define ADC_CHMASK 0x1F

/* 简短保持/建立时间（切通道后稍等） */
static void adc_delay(void)
{
    unsigned char i;
    for (i = 0; i < 20; ++i) { _nop_(); }
}

void ADC_Init(void)
{
    /* 打开 ADC 电源 */
    ADC_CONTR = ADC_POWER;

#ifdef P3ASF
    /* 仅启用需要的模拟通道：
       P3.4 → ADC12，P3.5 → ADC13，P3.6 → ADC14 */
    P3ASF |= (1u << 4) | (1u << 5) | (1u << 6);
#endif

    /* 把对应脚设为输入/高阻（PxM1=1, PxM0=0） */
    P3M1 |=  (1u << 4) | (1u << 5) | (1u << 6);
    P3M0 &= (unsigned char)~((1u << 4) | (1u << 5) | (1u << 6));

    /* 采样速度/对齐保持默认。如需调采样时钟、对齐等，可在此配置 ADCCFG。 */
}

unsigned int ADC_ReadRaw(unsigned char ch)
{
    unsigned int v;

    /* 选择通道并保持电源/标志位高位；清除低 5 位后写入 ch */
    ADC_CONTR = (ADC_CONTR & (ADC_POWER | ADC_FLAG)) | (ch & ADC_CHMASK);
    adc_delay();

    /* 启动转换 */
    ADC_CONTR |= ADC_START;

    /* 等待完成 */
    while (!(ADC_CONTR & ADC_FLAG)) { ; }

    /* 清完成标志（写 0 清）*/
    ADC_CONTR &= (unsigned char)~ADC_FLAG;

    /* 合成 10 位结果：ADC_RES[9:2] + ADC_RESL[1:0] */
    v = ((unsigned int)ADC_RES << 2) | (ADC_RESL & 0x03);
    return v;   /* 0..1023 */
}

unsigned int ADC_Read_mV(unsigned char ch)
{
    unsigned int raw = ADC_ReadRaw(ch);
    return (unsigned int)(((unsigned long)raw * (unsigned long)ADC_VREF_MV) / 1023ul);
}

unsigned int ADC_ReadAvg_mV(unsigned char ch, unsigned char times)
{
    unsigned long acc = 0;
    unsigned char n = (times == 0) ? 1 : (times > 32 ? 32 : times);
    unsigned char i;

    for (i = 0; i < n; ++i) {
        acc += ADC_Read_mV(ch);
    }
    return (unsigned int)(acc / n);
}
