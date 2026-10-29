#include "vitals.h"

#define VITALS_ANALYSIS_SAMPLES   400U
#define VITALS_MIN_SAMPLES        200U
#define VITALS_NO_FINGER_DC       1000.0f
#define VITALS_MIN_AC             10.0f
#define VITALS_MIN_PI             0.05f
#define VITALS_GOOD_PI            0.10f
#define VITALS_MIN_RR_SAMPLES     30
#define VITALS_MAX_RR_SAMPLES     200

static uint16_t raw_ir[VITALS_HISTORY_SIZE];
static uint16_t raw_red[VITALS_HISTORY_SIZE];
static float filtered_ir[VITALS_HISTORY_SIZE];
static float filtered_red[VITALS_HISTORY_SIZE];
static uint16_t history_head;
static uint16_t history_count;

static float ir_hp_prev;
static float ir_prev;
static float ir_lp;
static float red_hp_prev;
static float red_prev;
static float red_lp;

static uint8_t last_hr;
static uint8_t last_spo2;
static float last_pi;
static Vitals_Status last_status;

static uint16_t circular_index(uint16_t chronological_index, uint16_t window)
{
    uint16_t start = (uint16_t)((history_head + VITALS_HISTORY_SIZE - window) % VITALS_HISTORY_SIZE);
    return (uint16_t)((start + chronological_index) % VITALS_HISTORY_SIZE);
}

static float median_float(float *values, uint8_t count)
{
    uint8_t i;
    uint8_t j;
    float temp;
    for(i = 0U; i < count; i++)
    {
        for(j = (uint8_t)(i + 1U); j < count; j++)
        {
            if(values[j] < values[i])
            {
                temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }
    if(count & 1U)
    {
        return values[count / 2U];
    }
    return (values[count / 2U - 1U] + values[count / 2U]) * 0.5f;
}

void Vitals_Init(void)
{
    uint16_t i;
    for(i = 0U; i < VITALS_HISTORY_SIZE; i++)
    {
        raw_ir[i] = 0U;
        raw_red[i] = 0U;
        filtered_ir[i] = 0.0f;
        filtered_red[i] = 0.0f;
    }
    history_head = 0U;
    history_count = 0U;
    ir_hp_prev = 0.0f;
    ir_prev = 0.0f;
    ir_lp = 0.0f;
    red_hp_prev = 0.0f;
    red_prev = 0.0f;
    red_lp = 0.0f;
    last_hr = 0U;
    last_spo2 = 0U;
    last_pi = 0.0f;
    last_status = VITALS_STATUS_NO_FINGER;
}

void Vitals_AddSample(uint16_t red, uint16_t ir)
{
    float ir_value = (float)ir;
    float red_value = (float)red;
    float ir_hp;
    float red_hp;

    raw_ir[history_head] = ir;
    raw_red[history_head] = red;

    ir_hp = 0.969f * (ir_hp_prev + ir_value - ir_prev);
    ir_prev = ir_value;
    ir_hp_prev = ir_hp;
    ir_lp += 0.239f * (ir_hp - ir_lp);
    filtered_ir[history_head] = ir_lp;

    red_hp = 0.969f * (red_hp_prev + red_value - red_prev);
    red_prev = red_value;
    red_hp_prev = red_hp;
    red_lp += 0.239f * (red_hp - red_lp);
    filtered_red[history_head] = red_lp;

    history_head = (uint16_t)((history_head + 1U) % VITALS_HISTORY_SIZE);
    if(history_count < VITALS_HISTORY_SIZE)
    {
        history_count++;
    }
}

void Vitals_Update(void)
{
    uint16_t window;
    uint16_t i;
    uint16_t index;
    float sum_ir = 0.0f;
    float sum_red = 0.0f;
    float min_ir;
    float max_ir;
    float min_red;
    float max_red;
    float dc_ir;
    float dc_red;
    float ac_ir;
    float ac_red;
    float pi;
    float threshold;
    float rr[8];
    uint8_t rr_count = 0U;
    int16_t last_peak = -1000;
    uint8_t valid_hr = 0U;
    float hr_value = 0.0f;
    float ratio;
    float spo2_value;

    if(history_count < VITALS_MIN_SAMPLES)
    {
        last_hr = 0U;
        last_spo2 = 0U;
        last_pi = 0.0f;
        last_status = (history_count == 0U) ? VITALS_STATUS_NO_FINGER : VITALS_STATUS_ACQUIRING;
        return;
    }

    window = (history_count > VITALS_ANALYSIS_SAMPLES) ? VITALS_ANALYSIS_SAMPLES : history_count;
    index = circular_index(0U, window);
    min_ir = filtered_ir[index];
    max_ir = filtered_ir[index];
    min_red = filtered_red[index];
    max_red = filtered_red[index];

    for(i = 0U; i < window; i++)
    {
        index = circular_index(i, window);
        sum_ir += (float)raw_ir[index];
        sum_red += (float)raw_red[index];
        if(filtered_ir[index] < min_ir) min_ir = filtered_ir[index];
        if(filtered_ir[index] > max_ir) max_ir = filtered_ir[index];
        if(filtered_red[index] < min_red) min_red = filtered_red[index];
        if(filtered_red[index] > max_red) max_red = filtered_red[index];
    }

    dc_ir = sum_ir / (float)window;
    dc_red = sum_red / (float)window;
    ac_ir = max_ir - min_ir;
    ac_red = max_red - min_red;
    pi = (dc_ir > 0.0f) ? (ac_ir / dc_ir) * 100.0f : 0.0f;

    if((dc_ir < VITALS_NO_FINGER_DC) || (ac_ir < VITALS_MIN_AC))
    {
        last_hr = 0U;
        last_spo2 = 0U;
        last_pi = pi;
        last_status = VITALS_STATUS_NO_FINGER;
        return;
    }

    threshold = min_ir + 0.60f * (max_ir - min_ir);
    for(i = 1U; i + 1U < window; i++)
    {
        float previous = filtered_ir[circular_index((uint16_t)(i - 1U), window)];
        float current = filtered_ir[circular_index(i, window)];
        float next = filtered_ir[circular_index((uint16_t)(i + 1U), window)];
        if((current > previous) && (current >= next) && (current > threshold))
        {
            if((last_peak >= 0) && ((i - (uint16_t)last_peak) >= VITALS_MIN_RR_SAMPLES))
            {
                int16_t interval = (int16_t)(i - (uint16_t)last_peak);
                if((interval >= VITALS_MIN_RR_SAMPLES) && (interval <= VITALS_MAX_RR_SAMPLES) && (rr_count < 8U))
                {
                    rr[rr_count++] = (float)interval;
                }
                last_peak = (int16_t)i;
            }
            else if(last_peak < 0)
            {
                last_peak = (int16_t)i;
            }
        }
    }

    if(rr_count >= 2U)
    {
        float median_rr = median_float(rr, rr_count);
        if(median_rr > 0.0f)
        {
            hr_value = (60.0f * (float)VITALS_SAMPLE_RATE) / median_rr;
            if((hr_value >= 30.0f) && (hr_value <= 200.0f))
            {
                last_hr = (uint8_t)(hr_value + 0.5f);
                valid_hr = 1U;
            }
        }
    }
    if(!valid_hr)
    {
        last_hr = 0U;
    }

    ratio = 0.0f;
    if((dc_red > 1.0f) && (dc_ir > 1.0f) && (ac_ir > 0.1f))
    {
        ratio = (ac_red / dc_red) / (ac_ir / dc_ir);
    }

    if((ratio >= 0.30f) && (ratio <= 1.50f))
    {
        /* Demo coefficients only. Replace after reference-device calibration. */
        spo2_value = 104.0f - 17.0f * ratio;
        if(spo2_value < 70.0f) spo2_value = 70.0f;
        if(spo2_value > 100.0f) spo2_value = 100.0f;
        if(last_spo2 == 0U)
        {
            last_spo2 = (uint8_t)(spo2_value + 0.5f);
        }
        else
        {
            last_spo2 = (uint8_t)((0.70f * (float)last_spo2) + (0.30f * spo2_value) + 0.5f);
        }
    }
    else
    {
        last_spo2 = 0U;
    }

    last_pi = pi;
    if(pi < VITALS_MIN_PI)
    {
        last_status = VITALS_STATUS_WEAK;
    }
    else if((pi < VITALS_GOOD_PI) || (last_hr == 0U) || (last_spo2 == 0U))
    {
        last_status = VITALS_STATUS_WEAK;
    }
    else
    {
        last_status = VITALS_STATUS_OK;
    }
}

uint8_t Vitals_GetHeartRate(void)
{
    return last_hr;
}

uint8_t Vitals_GetSpO2(void)
{
    return last_spo2;
}

float Vitals_GetPerfusionIndex(void)
{
    return last_pi;
}

Vitals_Status Vitals_GetStatus(void)
{
    return last_status;
}

void Vitals_GetWaveform(uint8_t *out, uint8_t width)
{
    uint8_t i;
    uint16_t window;
    uint16_t index;
    float min_value;
    float max_value;
    float range;

    if((out == 0) || (width == 0U))
    {
        return;
    }
    if(history_count < width)
    {
        for(i = 0U; i < width; i++)
        {
            out[i] = 0U;
        }
        return;
    }

    window = width;
    index = circular_index(0U, window);
    min_value = filtered_ir[index];
    max_value = filtered_ir[index];
    for(i = 1U; i < width; i++)
    {
        index = circular_index(i, window);
        if(filtered_ir[index] < min_value) min_value = filtered_ir[index];
        if(filtered_ir[index] > max_value) max_value = filtered_ir[index];
    }

    range = max_value - min_value;
    for(i = 0U; i < width; i++)
    {
        if(range < 0.001f)
        {
            out[i] = 8U;
        }
        else
        {
            index = circular_index(i, window);
            out[i] = (uint8_t)(((filtered_ir[index] - min_value) * 15.0f / range) + 0.5f);
        }
    }
}
