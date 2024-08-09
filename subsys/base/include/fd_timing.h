#ifndef fd_timing_h
#define fd_timing_h

#include <stddef.h>
#include <stdint.h>

struct fd_timing_s;

typedef struct fd_timing_s {
    uint32_t count;
    double min_duration;
    double max_duration;
    double total_duration;
    double sum_squared_duration;

    uint32_t start;

    const char *name;
    struct fd_timing_s *next;
} fd_timing_t;

void fd_timing_initialize(void);

void fd_timing_register(fd_timing_t *timing, const char *name);
fd_timing_t *fd_timing_get_list(void);

double fd_timing_get_us_per_timestamp(void);

void fd_timing_enable(void);
uint32_t fd_timing_get_timestamp(void);
void fd_timing_disable(void);

void fd_timing_clear(fd_timing_t *timing);
void fd_timing_start(fd_timing_t *timing);
void fd_timing_end(fd_timing_t *timing);

void fd_timing_format(const fd_timing_t *timing, char *buffer, size_t size);

#endif
