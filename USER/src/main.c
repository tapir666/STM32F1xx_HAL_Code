/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-03-11 09:36:30
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-09-11 10:34:09
 * @FilePath: \STM32F1xx_HAL_Code\USER\src\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "stm32f1xx_hal.h"
#include "led.h"
#include "oled.h"
#include "max30102.h"
#include "vitals.h"

/**
  * @brief  System Clock Configuration
  *         System Clock source            = PLL (HSE)
  *         SYSCLK(Hz)                     = 72000000
  *         HCLK(Hz)                       = 72000000
  *         AHB Prescaler                  = 1
  *         APB1 Prescaler                 = 2
  *         APB2 Prescaler                 = 1
  *         PLLMUL                         = 9
  *         Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef oscinitstruct = {0};
  RCC_ClkInitTypeDef clkinitstruct = {0};

  /* 1. 配置外部晶振 + PLL */
  oscinitstruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  oscinitstruct.HSEState       = RCC_HSE_ON;
  oscinitstruct.HSIState       = RCC_HSI_ON;   // 保留 HSI，增强系统稳定性
  oscinitstruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  oscinitstruct.PLL.PLLState   = RCC_PLL_ON;
  oscinitstruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  oscinitstruct.PLL.PLLMUL     = RCC_PLL_MUL9; // 8MHz × 9 = 72MHz

  if (HAL_RCC_OscConfig(&oscinitstruct) != HAL_OK)
  {
    while (1);
  }

  /* 2. 选择 PLL 作为系统时钟，并配置总线分频 */
  clkinitstruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                            RCC_CLOCKTYPE_HCLK   |
                            RCC_CLOCKTYPE_PCLK1  |
                            RCC_CLOCKTYPE_PCLK2;

  clkinitstruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clkinitstruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   // HCLK = 72 MHz
  clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;     // PCLK1 = 36 MHz
  clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;     // PCLK2 = 72 MHz

  if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2) != HAL_OK)
  {
    while (1);
  }
}


static const char *status_text(Vitals_Status status)
{
	switch(status)
	{
		case VITALS_STATUS_OK:
			return "OK";
		case VITALS_STATUS_WEAK:
			return "WEAK";
		case VITALS_STATUS_ACQUIRING:
			return "ACQUIRING";
		default:
			return "NO FINGER";
	}
}

static uint16_t last_ir_value;
static uint16_t last_red_value;
static uint8_t last_fifo_result;
static uint8_t last_sample_count;

static void app_draw_waveform(void)
{
	uint8_t wave[128];
	uint8_t i;
	uint8_t y_prev;
	uint8_t y_cur;
	Vitals_GetWaveform(wave, 128U);
	for(i = 1U; i < 128U; i++)
	{
		y_prev = (uint8_t)(63U - (wave[i - 1U] & 0x0FU));
		y_cur = (uint8_t)(63U - (wave[i] & 0x0FU));
		if(y_prev <= y_cur)
		{
			OLED_DrawLine((u8)(i - 1U), y_prev, (u8)i, y_cur);
		}
		else
		{
			OLED_DrawLine((u8)(i - 1U), y_cur, (u8)i, y_prev);
		}
	}
}

static void app_display(void)
{
	uint8_t hr = Vitals_GetHeartRate();
	uint8_t spo2 = Vitals_GetSpO2();
	Vitals_Status status = Vitals_GetStatus();

	OLED_ClearBuffer();
	if(status == VITALS_STATUS_NO_FINGER)
	{
		uint8_t write_ptr = 0U;
		uint8_t read_ptr = 0U;
		uint8_t overflow = 0U;
		OLED_ShowString(0, 0, (u8 *)"SpO2: --", 16U);
		OLED_ShowString(0, 16, (u8 *)"BPM : --", 16U);
		if(last_fifo_result != 0U)
		{
			OLED_ShowString(0, 32, (u8 *)"I2C ERR:", 16U);
			OLED_ShowUInt(64, 32, last_fifo_result, 16U);
			OLED_ShowString(0, 48, (u8 *)"Check FIFO", 16U);
		}
		else if(last_sample_count == 0U)
		{
			if(MAX30102_GetFIFOStatus(&write_ptr, &read_ptr, &overflow) == 0U)
			{
				uint8_t mode = 0U;
				uint8_t spo2_cfg = 0U;
				uint8_t led1 = 0U;
				uint8_t led2 = 0U;
				uint8_t fifo_cfg = 0U;
				uint8_t part_id = 0U;
				if(MAX30102_ReadConfig(&mode, &spo2_cfg, &led1, &led2, &fifo_cfg, &part_id) == 0U)
				{
					OLED_ShowString(0, 0, (u8 *)"ID:", 16U);
					OLED_ShowHexByte(24, 0, part_id, 16U);
					OLED_ShowString(56, 0, (u8 *)"M:", 16U);
					OLED_ShowHexByte(72, 0, mode, 16U);
					OLED_ShowString(0, 16, (u8 *)"S:", 16U);
					OLED_ShowHexByte(16, 16, spo2_cfg, 16U);
					OLED_ShowString(48, 16, (u8 *)"F:", 16U);
					OLED_ShowHexByte(64, 16, fifo_cfg, 16U);
					OLED_ShowString(0, 32, (u8 *)"L1:", 16U);
					OLED_ShowHexByte(24, 32, led1, 16U);
					OLED_ShowString(56, 32, (u8 *)"L2:", 16U);
					OLED_ShowHexByte(80, 32, led2, 16U);
				}
				OLED_ShowString(0, 48, (u8 *)"W:", 16U);
				OLED_ShowUInt(16, 48, write_ptr, 16U);
				OLED_ShowString(40, 48, (u8 *)"R:", 16U);
				OLED_ShowUInt(56, 48, read_ptr, 16U);
				OLED_ShowString(80, 48, (u8 *)"O:", 16U);
				OLED_ShowUInt(96, 48, overflow, 16U);
			}
		}
		else
		{
			OLED_ShowString(0, 32, (u8 *)"IR:", 16U);
			OLED_ShowUInt(24, 32, last_ir_value, 16U);
			OLED_ShowString(0, 48, (u8 *)"RED:", 16U);
			OLED_ShowUInt(32, 48, last_red_value, 16U);
		}
		OLED_Refresh();
		return;
	}

	OLED_ShowString(0, 0, (u8 *)"SpO2:", 16U);
	if(spo2 > 0U)
	{
		OLED_ShowUInt(40, 0, spo2, 16U);
		OLED_ShowString(64, 0, (u8 *)"%", 16U);
	}
	else
	{
		OLED_ShowString(40, 0, (u8 *)"--", 16U);
	}

	OLED_ShowString(0, 16, (u8 *)"BPM :", 16U);
	if(hr > 0U)
	{
		OLED_ShowUInt(40, 16, hr, 16U);
	}
	else
	{
		OLED_ShowString(40, 16, (u8 *)"--", 16U);
	}

	OLED_ShowString(0, 32, (u8 *)"Status:", 16U);
	OLED_ShowString(56, 32, (u8 *)status_text(status), 16U);
	app_draw_waveform();
	OLED_Refresh();
}

static uint8_t app_value_abnormal(uint8_t hr, uint8_t spo2, Vitals_Status status)
{
	if(status != VITALS_STATUS_OK)
	{
		return 0U;
	}
	return (uint8_t)((hr < 50U) || (hr > 120U) || (spo2 < 90U));
}

int main(void)
{
	MAX30102_Sample samples[32];
	uint8_t sample_count = 0U;
	uint8_t init_result;
	uint32_t now;
	uint32_t last_fifo_read = 0U;
	uint32_t last_calc = 0U;
	uint32_t last_display = 0U;
	uint32_t last_led = 0U;
	uint32_t abnormal_since = 0U;
	uint8_t alarm_active = 0U;

	HAL_Init();
	SystemClock_Config();
	led_init();
	HAL_Delay(20U);
	D1_ON;
	D2_OFF;
	D3_OFF;
	D4_OFF;

	HAL_Delay(100U);
	OLED_Init();
	OLED_Clear();
	OLED_ShowString(0, 0, (u8 *)"MAX30102", 16U);
	OLED_ShowString(0, 16, (u8 *)"Init...", 16U);
	OLED_Refresh();
	HAL_Delay(200U);

	init_result = MAX30102_Init();
	if(init_result != 0U)
	{
		OLED_ClearBuffer();
		OLED_ShowString(0, 0, (u8 *)"MAX30102", 16U);
		OLED_ShowString(0, 16, (u8 *)"ERROR:", 16U);
		OLED_ShowUInt(56, 16, init_result, 16U);
		OLED_ShowString(0, 32, (u8 *)"Check I2C", 16U);
		OLED_Refresh();
		while(1)
		{
			D1_TOGGLE;
			HAL_Delay(250U);
		}
	}

	Vitals_Init();
	OLED_Clear();

	while(1)
	{
		now = HAL_GetTick();

		if((now - last_fifo_read) >= 20U)
		{
			last_fifo_read = now;
			last_fifo_result = MAX30102_ReadFIFO(samples, 32U, &sample_count);
			last_sample_count = sample_count;
			if(last_fifo_result == 0U)
			{
				uint8_t i;
				for(i = 0U; i < sample_count; i++)
				{
					Vitals_AddSample(samples[i].red, samples[i].ir);
					last_red_value = samples[i].red;
					last_ir_value = samples[i].ir;
				}
				if(sample_count > 0U)
				{
					D4_ON;
				}
			}
		}

		if((now - last_calc) >= 200U)
		{
			last_calc = now;
			Vitals_Update();
		}

		if((now - last_display) >= 200U)
		{
			last_display = now;
			app_display();
		}

		if((now - last_led) >= 500U)
		{
			last_led = now;
			if(Vitals_GetStatus() == VITALS_STATUS_OK)
			{
				D3_TOGGLE;
			}
			else
			{
				D3_OFF;
			}
			if(Vitals_GetStatus() == VITALS_STATUS_NO_FINGER)
			{
				D4_OFF;
			}
		}

		if(app_value_abnormal(Vitals_GetHeartRate(), Vitals_GetSpO2(), Vitals_GetStatus()))
		{
			if(abnormal_since == 0U)
			{
				abnormal_since = now;
			}
			if((now - abnormal_since) >= 5000U)
			{
				alarm_active = 1U;
			}
		}
		else
		{
			abnormal_since = 0U;
			alarm_active = 0U;
		}

		if(alarm_active)
		{
			D2_ON;
		}
		else
		{
			D2_OFF;
		}

		HAL_Delay(1U);
	}
}

