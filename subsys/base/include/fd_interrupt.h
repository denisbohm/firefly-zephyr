#ifndef fd_interrupt_h
#define fd_interrupt_h

#include <stdint.h>

uint32_t fd_interrupt_disable(void);
void fd_interrupt_enable(uint32_t state);
void fd_interrupt_wait(void);

#endif