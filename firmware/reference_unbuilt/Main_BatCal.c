#include "STC8H.h"
#include "config.h"
#include "UART.h"
#include "adc.h"
#include "delay.h"
#include "rgb.h"

#define ADC_CH_BAT              13
#define VBAT_SCALE_NUM          6U
#define VBAT_SCALE_DEN          1U
#define VBAT_GAIN_NUM           1101U
#define VBAT_GAIN_DEN           1000U

static void print_uint(unsigned long v)
{
    char buf[12];
    char *p = buf + sizeof(buf);

    *--p = '\0';
    if (v == 0) {
        *--p = '0';
    }
    while (v) {
        *--p = (char)('0' + (v % 10));
        v /= 10;
    }
    UART1_WriteString(p);
}
void main(void)
{
    unsigned long raw_acc;
    unsigned char i;
    unsigned int raw_avg;
    unsigned int pin_mv;
    unsigned long vbat_mv;

    delay_ms(300);
    UART1_Init_T2(FOSC_HZ, BAUD_RATE);
    UART1_WriteString("\r\n\r\n[CAL] Battery calibration firmware\r\n");
    UART1_WriteString("[CAL] Record multimeter VBAT together with each line below.\r\n");
    UART1_WriteString("[CAL] New gain = real_mV * 1000 / (pin_mV * 6)\r\n");

    ADC_Init();
    RGB_Init(1);
    RGB_Blue();

    while (1) {
        raw_acc = 0;
        for (i = 0; i < 32; ++i) {
            raw_acc += ADC_ReadRaw(ADC_CH_BAT);
        }

        raw_avg = (unsigned int)(raw_acc / 32UL);
        pin_mv = (unsigned int)((raw_acc * (unsigned long)ADC_VREF_MV) /
                                (32UL * 1023UL));
        vbat_mv = ((unsigned long)pin_mv * VBAT_SCALE_NUM * VBAT_GAIN_NUM) /
                  (VBAT_SCALE_DEN * VBAT_GAIN_DEN);

        UART1_WriteString("[CAL] raw=");
        print_uint(raw_avg);
        UART1_WriteString(", pin=");
        print_uint(pin_mv);
        UART1_WriteString(" mV, vbat_calc=");
        print_uint(vbat_mv);
        UART1_WriteString(" mV, scale_x1000=");
        print_uint(((unsigned long)VBAT_SCALE_NUM * VBAT_GAIN_NUM * 1000UL) /
                   ((unsigned long)VBAT_SCALE_DEN * VBAT_GAIN_DEN));
        UART1_WriteString("\r\n");

        delay_ms(1000);
    }
}
