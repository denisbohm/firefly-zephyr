#include "fd_timing.h"

#include <math.h>
#include <stdio.h>

void fd_timing_clear(fd_timing_t *timing) {
    timing->count = 0;
    timing->min_duration = 0;
    timing->max_duration = 0;
    timing->total_duration = 0.0;
    timing->sum_squared_duration = 0.0;
    timing->start = 0;
}

void fd_timing_start(fd_timing_t *timing) {
    timing->start = fd_timing_get_timestamp();
}

void fd_timing_end(fd_timing_t *timing) {
    uint32_t now = fd_timing_get_timestamp();
    uint32_t start = timing->start;
    double duration = (now < start) ? ((0xffffffff - start) + 1 + now) : (now - start);
    timing->total_duration += duration;
    timing->sum_squared_duration += duration * duration;
    timing->count += 1;
    if (timing->count == 1) {
        timing->min_duration = duration;
        timing->max_duration = duration;
    } else {
        if (duration < timing->min_duration) {
            timing->min_duration = duration;
        }
        if (duration > timing->max_duration) {
            timing->max_duration = duration;
        }
    }
}

void fd_timing_format(const fd_timing_t *timing, char *buffer, size_t size) {
    const double us_per_clock = 1.0 / 64.0;

    if (timing->count <= 0) {
        snprintf(buffer, size, "0 calls");
        return;
    }
    
    double min_time = timing->min_duration * us_per_clock;
    double max_time = timing->max_duration * us_per_clock;
    double mean_time = timing->total_duration * us_per_clock / timing->count;
    double standard_deviation = 0.0;
    double mean_squared = mean_time * mean_time;
    double mean_of_squares = timing->sum_squared_duration * us_per_clock * us_per_clock / timing->count;
    double variance = mean_of_squares - mean_squared;
    if (variance > 0.0) {
        standard_deviation = sqrt(variance);
    }
    snprintf(buffer, size, "%d calls, min %0.1f us, max %0.1f us, mean %0.1f us, stddev %0.1f us",
        timing->count, min_time, max_time, mean_time, standard_deviation);
}

void fd_timing_initialize(void) {
}
