#include "fd_adc.h"

#include "nrf.h"

void fd_adc_initialize(void) {
}

float fd_adc_convert(int channel, float max_voltage) {
    // Internal Reference 0.6 V
    // Gain 1/3

    int16_t result = 0;
    NRF_SAADC_S->ENABLE = 1;
    NRF_SAADC_S->CH[0].PSELP = channel << SAADC_CH_PSELP_PSELP_Pos;
    NRF_SAADC_S->CH[0].CONFIG = 
        (SAADC_CH_CONFIG_GAIN_Gain1_3 << SAADC_CH_CONFIG_GAIN_Pos) |
        (SAADC_CH_CONFIG_TACQ_40us << SAADC_CH_CONFIG_TACQ_Pos);
    NRF_SAADC_S->RESOLUTION = SAADC_RESOLUTION_VAL_12bit;
    NRF_SAADC_S->RESULT.PTR = (uint32_t)&result;
    NRF_SAADC_S->RESULT.MAXCNT = 1;
    NRF_SAADC_S->EVENTS_RESULTDONE = 0;
    NRF_SAADC_S->TASKS_START = 1;
    NRF_SAADC_S->TASKS_SAMPLE = 1;
    while (NRF_SAADC_S->EVENTS_RESULTDONE == 0) {
    }
    NRF_SAADC_S->TASKS_STOP = 1;
    NRF_SAADC_S->ENABLE = 0;
    float voltage = ((float)result) * 3.0f * 0.6f / 4096.0f;
    return voltage;
}
