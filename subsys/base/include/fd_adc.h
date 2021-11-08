#ifndef fd_adc_h
#define fd_adc_h

void fd_adc_initialize(void);

float fd_adc_convert(int channel, float max_voltage);

#endif
