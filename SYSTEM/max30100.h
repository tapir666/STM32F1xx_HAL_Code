#ifndef __MAX30100_H_
#define __MAX30100_H_

#include "stm32f1xx_hal.h"

#define MAX30100_I2C_ADDR        0x57U
#define MAX30100_ADDR_WRITE      ((MAX30100_I2C_ADDR << 1) | 0U)
#define MAX30100_ADDR_READ       ((MAX30100_I2C_ADDR << 1) | 1U)
#define MAX30100_PART_ID_VALUE   0x11U

typedef struct
{
    uint16_t red;
    uint16_t ir;
} MAX30100_Sample;

uint8_t MAX30100_Init(void);
uint8_t MAX30100_ReadPartID(uint8_t *part_id);
uint8_t MAX30100_ReadFIFO(MAX30100_Sample *samples, uint8_t max_samples, uint8_t *sample_count);
uint8_t MAX30100_WriteReg(uint8_t reg, uint8_t value);
uint8_t MAX30100_ReadReg(uint8_t reg, uint8_t *value);
void MAX30100_SetLEDCurrent(uint8_t red_current, uint8_t ir_current);

#endif
