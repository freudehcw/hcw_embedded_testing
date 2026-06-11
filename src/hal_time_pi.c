/*
 * hal_time_pi.c -- real timing HAL for Linux / Raspberry Pi.
 *
 * Monotonic microsecond clock via clock_gettime(CLOCK_MONOTONIC) and
 * busy/sleep waits via nanosleep. Compiled only into the target binary and
 * excluded from coverage (hardware/OS boundary).
 */
#define _POSIX_C_SOURCE 199309L

#include "hal_time.h"
#include <time.h>

uint64_t hal_time_now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

void hal_time_delay_us(uint32_t microseconds)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(microseconds / 1000000u);
    ts.tv_nsec = (long)(microseconds % 1000000u) * 1000L;
    (void)nanosleep(&ts, NULL);
}

void hal_time_delay_ms(uint32_t milliseconds)
{
    hal_time_delay_us(milliseconds * 1000u);
}
