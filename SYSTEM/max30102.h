#ifndef __MAX30102_H_
#define __MAX30102_H_

#include "stm32f1xx_hal.h"

#define MAX30102_I2C_ADDR        0x57U
#define MAX30102_ADDR_WRITE      ((MAX30102_I2C_ADDR << 1) | 0U)
#define MAX30102_ADDR_READ       ((MAX30102_I2C_ADDR << 1) | 1U)
#define MAX30102_PART_ID_VALUE   0x15U

typedef struct
{
    uint16_t red;
    uint16_t ir;
} MAX30102_Sample;

uint8_t MAX30102_Init(void);
uint8_t MAX30102_ReadPartID(uint8_t *part_id);
uint8_t MAX30102_ReadFIFO(MAX30102_Sample *samples, uint8_t max_samples, uint8_t *sample_count);
uint8_t MAX30102_GetFIFOStatus(uint8_t *write_ptr, uint8_t *read_ptr, uint8_t *overflow);
uint8_t MAX30102_ReadConfig(uint8_t *mode, uint8_t *spo2, uint8_t *led1,
                            uint8_t *led2, uint8_t *fifo, uint8_t *part_id);
uint8_t MAX30102_WriteReg(uint8_t reg, uint8_t value);
uint8_t MAX30102_ReadReg(uint8_t reg, uint8_t *value);
void MAX30102_SetLEDCurrent(uint8_t red_current, uint8_t ir_current);

#endif
