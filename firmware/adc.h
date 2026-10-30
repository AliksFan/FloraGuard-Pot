#ifndef __ADC_H__
#define __ADC_H__

#include "STC8H.h"

/* 参考电压（mV） */
#ifndef ADC_VREF_MV
#define ADC_VREF_MV  3300u
#endif

void         ADC_Init(void);
unsigned int ADC_ReadRaw(unsigned char ch);         /* 0..1023 */
unsigned int ADC_Read_mV(unsigned char ch);         /* mV */
unsigned int ADC_ReadAvg_mV(unsigned char ch, unsigned char times); /* mV, 1..32次 */

#endif
