#include "max30100.h"
#include "oled.h"

#define MAX30100_REG_INTERRUPT_STATUS   0x00U
#define MAX30100_REG_INTERRUPT_ENABLE   0x01U
#define MAX30100_REG_FIFO_WR_PTR        0x02U
#define MAX30100_REG_OVF_COUNTER        0x03U
#define MAX30100_REG_FIFO_RD_PTR        0x04U
#define MAX30100_REG_FIFO_DATA          0x05U
#define MAX30100_REG_MODE_CONFIG        0x06U
#define MAX30100_REG_SPO2_CONFIG        0x07U
#define MAX30100_REG_LED_CONFIG         0x09U
#define MAX30100_REG_TEMP_INTEGER       0x16U
#define MAX30100_REG_TEMP_FRACTION      0x17U
#define MAX30100_REG_REV_ID             0xFEU
#define MAX30100_REG_PART_ID            0xFFU

#define MAX30100_MODE_RESET             0x40U
#define MAX30100_MODE_SPO2              0x03U
#define MAX30100_SPO2_CONFIG_VALUE      0x47U
#define MAX30100_LED_CONFIG_VALUE       0x24U
#define MAX30100_FIFO_DEPTH             16U

static uint8_t MAX30100_ReadBytes(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t i;

    I2C_Start();
    Send_Byte(MAX30100_ADDR_WRITE);
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
    Send_Byte(MAX30100_ADDR_READ);
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

uint8_t MAX30100_WriteReg(uint8_t reg, uint8_t value)
{
    I2C_Start();
    Send_Byte(MAX30100_ADDR_WRITE);
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

uint8_t MAX30100_ReadReg(uint8_t reg, uint8_t *value)
{
    return MAX30100_ReadBytes(reg, value, 1U);
}

uint8_t MAX30100_ReadPartID(uint8_t *part_id)
{
    return MAX30100_ReadReg(MAX30100_REG_PART_ID, part_id);
}

void MAX30100_SetLEDCurrent(uint8_t red_current, uint8_t ir_current)
{
    uint8_t value = (uint8_t)(((red_current & 0x0FU) << 4) | (ir_current & 0x0FU));
    (void)MAX30100_WriteReg(MAX30100_REG_LED_CONFIG, value);
}

uint8_t MAX30100_Init(void)
{
    uint8_t part_id = 0U;
    uint8_t result;

    result = MAX30100_WriteReg(MAX30100_REG_MODE_CONFIG, MAX30100_MODE_RESET);
    if(result)
    {
        return 1U;
    }
    HAL_Delay(50U);

    result = MAX30100_ReadPartID(&part_id);
    if(result)
    {
        return 2U;
    }
    if(part_id != MAX30100_PART_ID_VALUE)
    {
        return 3U;
    }

    result = MAX30100_WriteReg(MAX30100_REG_INTERRUPT_ENABLE, 0x00U);
    if(result) return 4U;
    result = MAX30100_WriteReg(MAX30100_REG_FIFO_WR_PTR, 0x00U);
    if(result) return 5U;
    result = MAX30100_WriteReg(MAX30100_REG_OVF_COUNTER, 0x00U);
    if(result) return 6U;
    result = MAX30100_WriteReg(MAX30100_REG_FIFO_RD_PTR, 0x00U);
    if(result) return 7U;
    result = MAX30100_WriteReg(MAX30100_REG_SPO2_CONFIG, MAX30100_SPO2_CONFIG_VALUE);
    if(result) return 8U;
    result = MAX30100_WriteReg(MAX30100_REG_LED_CONFIG, MAX30100_LED_CONFIG_VALUE);
    if(result) return 9U;
    result = MAX30100_WriteReg(MAX30100_REG_MODE_CONFIG, MAX30100_MODE_SPO2);
    if(result) return 10U;

    HAL_Delay(20U);
    return 0U;
}

uint8_t MAX30100_ReadFIFO(MAX30100_Sample *samples, uint8_t max_samples, uint8_t *sample_count)
{
    uint8_t write_ptr = 0U;
    uint8_t read_ptr = 0U;
    uint8_t overflow = 0U;
    uint8_t available;
    uint8_t i;
    uint8_t raw[4];

    *sample_count = 0U;

    if(MAX30100_ReadReg(MAX30100_REG_FIFO_WR_PTR, &write_ptr)) return 1U;
    if(MAX30100_ReadReg(MAX30100_REG_OVF_COUNTER, &overflow)) return 2U;
    if(MAX30100_ReadReg(MAX30100_REG_FIFO_RD_PTR, &read_ptr)) return 3U;

    if(overflow > 0U)
    {
        available = MAX30100_FIFO_DEPTH;
    }
    else
    {
        available = (uint8_t)((write_ptr + MAX30100_FIFO_DEPTH - read_ptr) & 0x0FU);
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
        if(MAX30100_ReadBytes(MAX30100_REG_FIFO_DATA, raw, 4U))
        {
            return 4U;
        }
        samples[i].ir = (uint16_t)(((uint16_t)raw[0] << 8) | raw[1]);
        samples[i].red = (uint16_t)(((uint16_t)raw[2] << 8) | raw[3]);
    }

    if(overflow > 0U)
    {
        read_ptr = write_ptr;
    }
    else
    {
        read_ptr = (uint8_t)((read_ptr + available) & 0x0FU);
    }
    (void)MAX30100_WriteReg(MAX30100_REG_FIFO_RD_PTR, read_ptr);
    if(overflow > 0U)
    {
        (void)MAX30100_WriteReg(MAX30100_REG_OVF_COUNTER, 0x00U);
    }

    *sample_count = available;
    return 0U;
}
