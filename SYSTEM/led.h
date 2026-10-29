#ifndef __LED_H_
#define __LED_H_

#include "stm32f1xx_hal.h"
/*灯的分组和编号*/
#define D1_GROUP GPIOA 
#define D2_3_4_GROUP GPIOB 
#define D1_PIN GPIO_PIN_15
#define D2_PIN GPIO_PIN_3
#define D3_PIN GPIO_PIN_4
#define D4_PIN GPIO_PIN_5

void led_init(void);

#define D1_ON       HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET)
#define D1_OFF      HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET)
#define D1_TOGGLE   HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_15)

#define D2_ON       HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_RESET)
#define D2_OFF      HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_SET)
#define D2_TOGGLE   HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_3)

#define D3_ON       HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET)
#define D3_OFF      HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET)
#define D3_TOGGLE   HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_4)

#define D4_ON       HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_RESET)
#define D4_OFF      HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET)
#define D4_TOGGLE   HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_5)

#define LED_ALL_ON      (D1_ON,D2_ON,D3_ON,D4_ON)
#define LED_ALL_OFF     (D1_OFF,D2_OFF,D3_OFF,D4_OFF)
#define LED_ALL_TOGGLE  (D1_TOGGLE,D2_TOGGLE,D3_TOGGLE,D4_TOGGLE)

#endif
