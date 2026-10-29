/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-09-08 20:06:55
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-09-11 09:49:07
 * @FilePath: \STM32F1xx_HAL_Code\SYSTEM\led.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "led.h"
void led_init(void)
{
	GPIO_InitTypeDef GPIO_Init;//定义了一个GPIO结构体变量

  __HAL_RCC_AFIO_CLK_ENABLE();         //开启AFIO复用时钟
  __HAL_AFIO_REMAP_SWJ_NOJTAG();      //关闭JTAG，释放PA15 PB3 PB4；SWD下载仍然可用

  //1.使能GPIOF组和GPIOE组的时钟
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	//2.GPIO引脚初始化
	GPIO_Init.Pin = D1_PIN;//同组且功能模式完全一样
	GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;//推挽输出
	GPIO_Init.Pull = GPIO_NOPULL;//不需要上下拉
	GPIO_Init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(D1_GROUP,&GPIO_Init);//上述的结构体的数据才会写入到寄存器中

	GPIO_Init.Pin = D2_PIN|D3_PIN|D4_PIN;//同组且功能模式完全一样
	HAL_GPIO_Init(D2_3_4_GROUP,&GPIO_Init);
		
	//3.D1处于默认状态 -->灯灭 
	HAL_GPIO_WritePin(D1_GROUP,D1_PIN,GPIO_PIN_SET);
	HAL_GPIO_WritePin(D2_3_4_GROUP,D2_PIN|D3_PIN|D4_PIN,GPIO_PIN_SET);
}
