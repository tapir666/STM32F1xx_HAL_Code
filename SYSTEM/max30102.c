#include "max30102.h"
#include "oled.h"

#define MAX30102_REG_INTERRUPT_STATUS   0x00U
#define MAX30102_REG_INTERRUPT_ENABLE   0x01U
#define MAX30102_REG_FIFO_WR_PTR        0x04U
#define MAX30102_REG_OVF_COUNTER        0x05U
#define MAX30102_REG_FIFO_RD_PTR        0x06U
#define MAX30102_REG_FIFO_DATA          0x07U
#define MAX30102_REG_FIFO_CONFIG        0x08U
#define MAX30102_REG_MODE_CONFIG        0x09U
#define MAX30102_REG_SPO2_CONFIG        0x0AU
#define MAX30102_REG_LED1_PA            0x0CU
#define MAX30102_REG_LED2_PA            0x0DU
#define MAX30102_REG_PILOT_PA           0x10U
#define MAX30102_REG_TEMP_INTEGER       0x1FU
#define MAX30102_REG_TEMP_FRACTION      0x20U
#define MAX30102_REG_TEMP_CONFIG        0x21U
#define MAX30102_REG_REV_ID             0xFEU
#define MAX30102_REG_PART_ID            0xFFU

#define MAX30102_MODE_RESET             0x40U
#define MAX30102_MODE_SPO2              0x03U
#define MAX30102_FIFO_CONFIG_VALUE      0x0FU
#define MAX30102_SPO2_CONFIG_VALUE      0x27U
#define MAX30102_LED_CURRENT_VALUE      0x24U
#define MAX30102_FIFO_DEPTH             32U

static uint8_t MAX30102_ReadBytes(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t i;

    I2C_Start();
    Send_Byte(MAX30102_ADDR_WRITE);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 1U;
    }

    Send_Byte(reg);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 2U;
    }

    I2C_Start();
    Send_Byte(MAX30102_ADDR_READ);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 3U;
    }

    for(i = 0U; i < length; i++)
    {
        data[i] = I2C_Read_Byte((i + 1U) < length);
    }
    I2C_Stop();
    return 0U;
}

uint8_t MAX30102_WriteReg(uint8_t reg, uint8_t value)
{
    I2C_Start();
    Send_Byte(MAX30102_ADDR_WRITE);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 1U;
    }

    Send_Byte(reg);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 2U;
    }

    Send_Byte(value);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return 3U;
    }

    I2C_Stop();
    return 0U;
}

uint8_t MAX30102_ReadReg(uint8_t reg, uint8_t *value)
{
    return MAX30102_ReadBytes(reg, value, 1U);
}

uint8_t MAX30102_ReadPartID(uint8_t *part_id)
{
    return MAX30102_ReadReg(MAX30102_REG_PART_ID, part_id);
}

uint8_t MAX30102_GetFIFOStatus(uint8_t *write_ptr, uint8_t *read_ptr, uint8_t *overflow)
{
    if(MAX30102_ReadReg(MAX30102_REG_FIFO_WR_PTR, write_ptr)) return 1U;
    if(MAX30102_ReadReg(MAX30102_REG_OVF_COUNTER, overflow)) return 2U;
    if(MAX30102_ReadReg(MAX30102_REG_FIFO_RD_PTR, read_ptr)) return 3U;
    *write_ptr &= 0x1FU;
    *read_ptr &= 0x1FU;
    *overflow &= 0x1FU;
    return 0U;
}

uint8_t MAX30102_ReadConfig(uint8_t *mode, uint8_t *spo2, uint8_t *led1,
                            uint8_t *led2, uint8_t *fifo, uint8_t *part_id)
{
    if(MAX30102_ReadReg(MAX30102_REG_MODE_CONFIG, mode)) return 1U;
    if(MAX30102_ReadReg(MAX30102_REG_SPO2_CONFIG, spo2)) return 2U;
    if(MAX30102_ReadReg(MAX30102_REG_LED1_PA, led1)) return 3U;
    if(MAX30102_ReadReg(MAX30102_REG_LED2_PA, led2)) return 4U;
    if(MAX30102_ReadReg(MAX30102_REG_FIFO_CONFIG, fifo)) return 5U;
    if(MAX30102_ReadReg(MAX30102_REG_PART_ID, part_id)) return 6U;
    return 0U;
}

void MAX30102_SetLEDCurrent(uint8_t red_current, uint8_t ir_current)
{
    (void)MAX30102_WriteReg(MAX30102_REG_LED1_PA, red_current);
    (void)MAX30102_WriteReg(MAX30102_REG_LED2_PA, ir_current);
}

uint8_t MAX30102_Init(void)
{
    uint8_t part_id = 0U;
    uint8_t result;

    result = MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_RESET);
    if(result)
    {
        return 1U;
    }
    HAL_Delay(50U);

    result = MAX30102_ReadPartID(&part_id);
    if(result)
    {
        return 2U;
    }
    if(part_id != MAX30102_PART_ID_VALUE)
    {
        return 3U;
    }

    result = MAX30102_WriteReg(MAX30102_REG_INTERRUPT_ENABLE, 0x00U);
    if(result) return 4U;
    result = MAX30102_WriteReg(MAX30102_REG_FIFO_WR_PTR, 0x00U);
    if(result) return 5U;
    result = MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00U);
    if(result) return 6U;
    result = MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, 0x00U);
    if(result) return 7U;
    result = MAX30102_WriteReg(MAX30102_REG_FIFO_CONFIG, MAX30102_FIFO_CONFIG_VALUE);
    if(result) return 8U;
    result = MAX30102_WriteReg(MAX30102_REG_SPO2_CONFIG, MAX30102_SPO2_CONFIG_VALUE);
    if(result) return 9U;
    result = MAX30102_WriteReg(MAX30102_REG_LED1_PA, MAX30102_LED_CURRENT_VALUE);
    if(result) return 10U;
    result = MAX30102_WriteReg(MAX30102_REG_LED2_PA, MAX30102_LED_CURRENT_VALUE);
    if(result) return 11U;
    result = MAX30102_WriteReg(MAX30102_REG_PILOT_PA, 0x7FU);
    if(result) return 12U;
    result = MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SPO2);
    if(result) return 13U;

    HAL_Delay(20U);
    return 0U;
}

uint8_t MAX30102_ReadFIFO(MAX30102_Sample *samples, uint8_t max_samples, uint8_t *sample_count)
{
    uint8_t write_ptr = 0U;
    uint8_t read_ptr = 0U;
    uint8_t overflow = 0U;
    uint8_t available;
    uint8_t i;
    uint8_t raw[6];
    uint32_t red;
    uint32_t ir;

    *sample_count = 0U;

    if(MAX30102_ReadReg(MAX30102_REG_FIFO_WR_PTR, &write_ptr)) return 1U;
    if(MAX30102_ReadReg(MAX30102_REG_OVF_COUNTER, &overflow)) return 2U;
    if(MAX30102_ReadReg(MAX30102_REG_FIFO_RD_PTR, &read_ptr)) return 3U;

    write_ptr &= 0x1FU;
    read_ptr &= 0x1FU;

    if(overflow > 0U)
    {
        available = MAX30102_FIFO_DEPTH;
    }
    else
    {
        available = (uint8_t)((write_ptr + MAX30102_FIFO_DEPTH - read_ptr) & 0x1FU);
    }

    if(available > max_samples)
    {
        available = max_samples;
    }
    if(available == 0U)
    {
        return 0U;
    }

    for(i = 0U; i < available; i++)
    {
        if(MAX30102_ReadBytes(MAX30102_REG_FIFO_DATA, raw, 6U))
        {
            return 4U;
        }

        /* FIFO order is RED[17:0], then IR[17:0]. Data is left-justified. */
        red = ((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | raw[2];
        ir = ((uint32_t)raw[3] << 16) | ((uint32_t)raw[4] << 8) | raw[5];
        samples[i].red = (uint16_t)(red >> 2);
        samples[i].ir = (uint16_t)(ir >> 2);
    }

    if(overflow > 0U)
    {
        read_ptr = write_ptr;
    }
    else
    {
        read_ptr = (uint8_t)((read_ptr + available) & 0x1FU);
    }
    (void)MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, read_ptr);
    if(overflow > 0U)
    {
        (void)MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00U);
    }

    *sample_count = available;
    return 0U;
}
