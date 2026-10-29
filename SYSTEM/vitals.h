#ifndef __VITALS_H_
#define __VITALS_H_

#include "stm32f1xx_hal.h"

#define VITALS_SAMPLE_RATE       100U
#define VITALS_HISTORY_SIZE      512U

typedef enum
{
    VITALS_STATUS_NO_FINGER = 0,
    VITALS_STATUS_ACQUIRING,
    VITALS_STATUS_OK,
    VITALS_STATUS_WEAK
} Vitals_Status;

void Vitals_Init(void);
void Vitals_AddSample(uint16_t red, uint16_t ir);
void Vitals_Update(void);
uint8_t Vitals_GetHeartRate(void);
uint8_t Vitals_GetSpO2(void);
float Vitals_GetPerfusionIndex(void);
Vitals_Status Vitals_GetStatus(void);
void Vitals_GetWaveform(uint8_t *out, uint8_t width);

#endif
