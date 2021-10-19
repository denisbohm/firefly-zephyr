#ifndef fd_ieee754_h
#define fd_ieee754_h

#include <stdint.h>

float fd_ieee754_uint32_to_float(uint32_t value);
uint32_t fd_ieee754_float_to_uint32(float value);

uint16_t fd_ieee754_float_to_uint16(float value);
float fd_ieee754_uint16_to_float(uint16_t value);

#endif
