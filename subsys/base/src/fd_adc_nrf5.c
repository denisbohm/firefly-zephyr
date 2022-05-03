#include "fd_adc.h"

#include "nrf.h"

void fd_adc_initialize(void) {
}

float fd_adc_convert(int channel, float max_voltage) {
    // Internal Reference 0.6 V
    // Gain 1/3

    uint32_t result = 0;
    NRF_SAADC_Type *nrf_saadc;
#ifdef NRF52_SERIES
    nrf_saadc = NRF_SAADC;
#endif
#ifdef NRF53_SERIES
    nrf_saadc = NRF_SAADC_S;
#endif
    nrf_saadc->ENABLE = 1;
    nrf_saadc->CH[0].PSELP = channel << SAADC_CH_PSELP_PSELP_Pos;
    nrf_saadc->CH[0].CONFIG = 
        (SAADC_CH_CONFIG_GAIN_Gain1_3 << SAADC_CH_CONFIG_GAIN_Pos) |
        (SAADC_CH_CONFIG_TACQ_40us << SAADC_CH_CONFIG_TACQ_Pos);
    nrf_saadc->RESOLUTION = SAADC_RESOLUTION_VAL_12bit;
    nrf_saadc->RESULT.PTR = (uint32_t)&result;
    nrf_saadc->RESULT.MAXCNT = 1;
    nrf_saadc->EVENTS_RESULTDONE = 0;
    nrf_saadc->TASKS_START = 1;
    nrf_saadc->TASKS_SAMPLE = 1;
    while (nrf_saadc->EVENTS_RESULTDONE == 0) {
    }
    nrf_saadc->EVENTS_STOPPED = 0;
    nrf_saadc->TASKS_STOP = 1;
    while (nrf_saadc->EVENTS_STOPPED == 0) {
    }
    nrf_saadc->ENABLE = 0;
    int16_t value = (uint16_t)(result & 0xffff);
    if (value < 0) {
        static int below_zero = 0;
        ++below_zero;
    }
    float voltage = ((float)value) * 3.0f * 0.6f / 4096.0f;
    return voltage;
}
