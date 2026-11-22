# STM32 ADC 单通道采样

## 项目简介

基于 STM32F103 + 标准库，用 **ADC1 通道 0（PA0）**做单次转换，读取 0–3.3V 模拟电压，换算后显示在 OLED 上。

## 实验现象

- OLED 第 2 行显示电压，例如 `1.65`
- 旋动电位器（或改变输入电压），数值实时变化
- 3.3V 输入时显示约 `3.30`，0V 时显示 `0.00`

## 硬件

| 连接 | 说明 |
|---|---|
| PA0 | ADC1_IN0，接电位器中点或 0–3.3V 信号 |
| 电位器两端 | 分别接 3.3V 和 GND |
| GND | 必须共地 |

> ⚠️ 输入电压**不能超过 3.3V**（VDDA），否则可能损坏引脚。

## 参数计算

```
ADCCLK = PCLK2 / 6 = 72MHz / 6 = 12MHz   （不超过 14MHz 上限）
采样时间 = 55.5 个 ADC 周期
转换时间 ≈ (55.5 + 12.5) / 12MHz ≈ 5.7µs
```

| 项目 | 值 |
|---|---|
| 分辨率 | 12 位（0–4095） |
| 参考电压 | 3.3V（VDDA） |
| 电压换算 | `V = 原始值 × 3.3 / 4095` |

## 关键代码

`Hardware/ad.c`：

```c
void adc_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);        /* 12MHz */

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);

    ADC_InitTypeDef ADC_InitStruct;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;   /* 单次转换 */
    ADC_InitStruct.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStruct.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStruct.ADC_NbrOfChannel       = 1;
    ADC_InitStruct.ADC_ScanConvMode       = DISABLE;
    ADC_Init(ADC1, &ADC_InitStruct);

    ADC_Cmd(ADC1, ENABLE);

    /* 必须先校准，否则精度差 */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET);
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET);
}

uint16_t ad_start(void)
{
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}
```

`User/main.c`：

```c
int main(void)
{
    OLED_Init();
    adc_init();

    while (1)
    {
        voltage = ad_start();
        truev   = (float)voltage * 3.3f / 4095;
        OLED_ShowNum(2, 2, truev, 1);                    /* 整数部分 */
        OLED_ShowNum(2, 4, (int)(truev * 100) % 100, 2); /* 小数部分 */
    }
}
```

## 踩过的坑

- **忘记校准**：不校准会有几十 mV 偏差
- **ADCCLK 超 14MHz**：72MHz 直连会超频，必须分频（Div6 → 12MHz）
- **PA0 模式设成 `GPIO_Mode_AIN`**：模拟输入，不能是浮空/上拉
- **EOC 标志**：软件触发后必须等 `ADC_FLAG_EOC`，否则读到旧值
- **参考电压不是绝对 3.3V**：实际 VDDA 可能是 3.28V，要更准需用万用表校准

## 下一步

- [ ] 多通道（PA0–PA7）轮流采样
- [ ] DMA + 连续转换（不占 CPU）
- [ ] 内部温度传感器 / 参考电压读取
- [ ] 给小车做电池电压检测（分压 + ADC）

## 开发环境

- Keil MDK uVision 5
- STM32F10x 标准外设库
- 烧录：ST-Link / FlyMcu
