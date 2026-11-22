#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "ad.h"
volatile uint16_t voltage;
volatile float truev;
int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	
	adc_init();
	
	while (1)
	{
		voltage=ad_start();
		truev=(float)voltage*3.3/4095;
		OLED_ShowNum(2,2,truev,1);
		OLED_ShowNum(2,4,(int)(truev*100)%100,2);
	}
}

