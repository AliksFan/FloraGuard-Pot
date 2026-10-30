#include "STC8H.h"
#include "config.h"
#include "UART.h"
#include "adc.h"
#include "io.h"
#include "delay.h"
#include "PWM.h"
#include "rgb.h"
#include "sleep.h"
#include "isr.h"


/*================= ADC 通道与参数 =================*/
#define ADC_CH_BAT              13    /* P3.5 -> ADC13 (电池分压) */
#define ADC_CH_POT              12    /* P3.4 -> ADC12 (电位器 0~3.3V) */
#define ADC_CH_SOIL             14    /* P3.6 -> ADC14 (土壤 0~2000mV) */

#define ADC_AVG_TIMES           16U

/* 分压比例 + 你的增益校准 */
#define VBAT_SCALE_NUM          6U
#define VBAT_SCALE_DEN          1U
#define VBAT_GAIN_NUM           1101U
#define VBAT_GAIN_DEN           1000U

/*================= 前置声明 =================*/
static void         Board_Init(void);
static unsigned int Measure_Battery_mV(void);
static void         Battery_Calibration_Run(void);

/* 小工具：打印无符号整数（避免 printf） */
static void print_uint(unsigned long v)
{
    char buf[12]; char *p = buf + sizeof(buf);
    *--p = '\0';
    if (v == 0) *--p = '0';
    while (v) { *--p = (char)('0' + (v % 10)); v /= 10; }
    UART1_WriteString(p);
}

/* 线性映射并限幅 */
static unsigned int map_clamp(unsigned long x,
                              unsigned long in_min,  unsigned long in_max,
                              unsigned long out_min, unsigned long out_max)
{
    if (x <= in_min) return (unsigned int)out_min;
    if (x >= in_max) return (unsigned int)out_max;
    return (unsigned int)( out_min +
        ( (x - in_min) * (out_max - out_min) ) / (in_max - in_min) );
}

/* ===== 土壤湿度评估 =====
 * Return:
 *   1 = No watering needed
 *   2 = OK to water
 *   3 = Dry, needs water
 */
static unsigned char Soil_CheckAndDecide(void)
{
    unsigned int pot_mv, soil_mv;
    unsigned int pot_scale;   /* 1..100 */
    unsigned int soil_scale;  /* 0..100 */
    unsigned char status;

    /* 打开外设电源（P1.1：低=开） */
    IO_PowerOn();
    UART1_WriteString("[SOIL] PowerOn\r\n");

    /* 150kHz/50% 激励 */
    PWM_Start(1);
    UART1_WriteString("[PWM] start\r\n");
	
    delay_ms(3000);

    /* 读取电位器（0~3300mV -> 1..100；你的算法里反相再回正） */
    pot_mv    = ADC_ReadAvg_mV(ADC_CH_POT,  (unsigned char)ADC_AVG_TIMES);
    pot_scale = 100 - map_clamp(pot_mv, 0, 3300, 1, 100);
		
	
    UART1_WriteString("[SOIL] POT  scale=");
    print_uint(pot_scale);
    UART1_WriteString(", V=");
    print_uint(pot_mv);
    UART1_WriteString(" mV\r\n");

    /* 读取土壤（0~2000mV -> 0..100；也按你的算法反相） */
    soil_mv    = ADC_ReadAvg_mV(ADC_CH_SOIL, (unsigned char)ADC_AVG_TIMES);
    soil_scale = 100 - map_clamp(soil_mv, 0, 2000, 0, 100);

    UART1_WriteString("[SOIL] SOIL scale=");
    print_uint(soil_scale);
    UART1_WriteString(", V=");
    print_uint(soil_mv);
    UART1_WriteString(" mV\r\n");

    PWM_Stop(1);
    UART1_WriteString("[PWM] stop\r\n");
		
		IO_PowerOff();
		UART1_WriteString("[SOIL] PowerOff\r\n");

    /* 判定：把目标阈值回正到 1..100 比较 */
    
    if (soil_scale >= pot_scale) {
        status = 1;  /* 更湿，不需要浇水 */
    } else if (soil_scale < pot_scale) {
        status = 2;  /* 缺水 */
    } else {
        status = 1;  /* 在 ±10 内，可以浇水 */
    }

    

    UART1_WriteString("[SOIL] Decision=");
    print_uint(status);
    UART1_WriteString(" (1=NoWater, 2=OKToWater, 3=Dry)\r\n");

    return status;
}

/*================= Battery calibration mode =================*/
static void Battery_Calibration_Run(void)
{
    unsigned long raw_acc;
    unsigned char i;
    unsigned int raw_avg;
    unsigned int pin_mv;
    unsigned long vbat_mv;

    UART1_Init_T2(FOSC_HZ, BAUD_RATE);
    UART1_WriteString("\r\n\r\n[CAL] Battery calibration mode\r\n");
    UART1_WriteString("[CAL] Measure real VBAT with a multimeter, then record raw/pin/vbat_calc.\r\n");

    ADC_Init();
    RGB_Init(1);
    RGB_Blue();

    while (1)
    {
        raw_acc = 0;
        for (i = 0; i < 32; ++i) {
            raw_acc += ADC_ReadRaw(ADC_CH_BAT);
        }

        raw_avg = (unsigned int)(raw_acc / 32UL);
        pin_mv = (unsigned int)((raw_acc * (unsigned long)ADC_VREF_MV) / (32UL * 1023UL));
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
/*================= 主函数 =================*/
int main(void)
{
#if BATTERY_CALIBRATION_MODE
    delay_ms(300);
    Battery_Calibration_Run();
#else
    unsigned int vbat_mv;
    unsigned char soil_status;

    delay_ms(300);
    UART1_WriteString("\r\n[BOOT] Init start.\r\n");

    Board_Init();

    while (1)
    {
        /* 初始化后：点亮绿灯（提示开始一轮检测） */
        RGB_Green();

        /* 1) 电池电压测量并保存 */
        UART1_WriteString("[BOOT] Measuring battery...\r\n");
        vbat_mv = Measure_Battery_mV();
        UART1_WriteString("[BAT] VBAT=");
        print_uint(vbat_mv);
        UART1_WriteString(" mV\r\n");
			
				delay_ms(1000);
        /* 2) 土壤检测并保存 */
        soil_status = Soil_CheckAndDecide();  /* 1/2/3 */
				

        /* ====== 按表格执行 ======
           列阈值：
             正常        : vbat >= 3650
             低于3.65V   : 3450 <= vbat < 3650
             低于3.45V   : vbat < 3450
           行：
             soil=1 (NoWater) -> 绿灯 / 3s / 永久休眠
             soil=2/3 (OK/Dry) -> 红蓝 / 红蓝交替3s / 永久休眠
         */
        if (vbat_mv < 3450U)
        {
            UART1_WriteString("[ACTION] VBAT < 3.45V -> Sleep forever\r\n");
            RGB_Off();
            Sleep_Forever();              /* 永久掉电 */
            /* 不再返回 */
        }
        else if (vbat_mv < 3650U)
        {
            if (soil_status == 1)
            {
                UART1_WriteString("[ACTION] VBAT < 3.65V & NoWater -> Red 3s pattern (12h)\r\n");
                Sleep_Red_3s_for_12h();
            }
            else
            {
                UART1_WriteString("[ACTION] VBAT < 3.65V & (OK/Dry) -> Red/Blue alternate (12h)\r\n");
                Sleep_Alt_RedBlue_3s_for_12h();
            }
            /* 返回后回到循环开头：再次亮绿灯并重新检测 */
        }
        else
        {
            if (soil_status == 1)
            {
                UART1_WriteString("[ACTION] VBAT normal & NoWater -> Green 6s pattern (12h)\r\n");
                Sleep_Green_6s_for_12h();
            }
            else
            {
                UART1_WriteString("[ACTION] VBAT normal & (OK/Dry) -> Blue 3s pattern (12h)\r\n");
                Sleep_Blue_3s_for_12h();
            }
            /* 返回后回到循环开头 */
        }
    }
#endif
}

/*================= Init =================*/
static void Board_Init(void)
{
    UART1_Init_T2(FOSC_HZ, BAUD_RATE);
    UART1_WriteString("\r\n\r\n[BOOT] UART ready\r\n");

    ADC_Init();
    UART1_WriteString("[BOOT] ADC ready (P3.4/3.5/3.6)\r\n");
	
	/* RGB：共阳 = 1；若为共阴，置 0 */
    RGB_Init(1);
    RGB_Off();

    IO_Init();  /* 现在只初始化 P1.1 供电控制 */
    UART1_WriteString("[BOOT] IO ready (P1.1 power switch)\r\n");

		ISR_Init_All();         /* 开启 INT3/0/1 + T2 + EA */
    

    /* PWM：用于土壤测量时的激励（默认 150 kHz / 50%） */
    PWM_Init(150000);
    PWM_SetDuty(1, 50);
    UART1_WriteString("[BOOT] PWM ready: 150kHz, duty=50%\r\n");
	
//		//    /* 打开外设电源（P1.1：低=开） */
//    IO_PowerOn();
//    UART1_WriteString("[SOIL] PowerOn\r\n");

//    /* 150kHz/50% 激励 */
//    PWM_Start(1);
//    UART1_WriteString("[PWM] start\r\n");
	
}

/*================= Battery measure =================*/
static unsigned int Measure_Battery_mV(void)
{
    unsigned int vpin_mv = ADC_ReadAvg_mV(ADC_CH_BAT, (unsigned char)ADC_AVG_TIMES);
    unsigned long vbat_mv =
        ((unsigned long)vpin_mv * VBAT_SCALE_NUM * VBAT_GAIN_NUM) /
        (VBAT_SCALE_DEN * VBAT_GAIN_DEN);

    UART1_WriteString("[ADC] pin=");
    {
        unsigned int v = vpin_mv;
        char buf[10]; char *p = buf + sizeof(buf); *--p = '\0';
        if (v == 0) *--p = '0';
        while (v) { *--p = (char)('0' + (v % 10)); v /= 10; }
        UART1_WriteString(p);
    }
    UART1_WriteString(" mV, VBAT=");
    {
        unsigned long vv = vbat_mv;
        char buf[12]; char *p = buf + sizeof(buf); *--p = '\0';
        if (vv == 0) *--p = '0';
        while (vv) { *--p = (char)('0' + (vv % 10)); vv /= 10; }
        UART1_WriteString(p);
    }
    UART1_WriteString(" mV\r\n");

    return (unsigned int)vbat_mv;
}
