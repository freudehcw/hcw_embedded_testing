/*
 * hal_time.h -- Hardware Abstraction Layer for timing.
 *
 * Like hal_gpio.h, this header is a mock seam. The HC-SR04 driver needs a
 * monotonic microsecond clock to measure the echo pulse, and the buzzer
 * driver needs precise busy-wait delays to bit-bang a square wave. Both are
 * abstracted here so the timing behaviour can be driven deterministically
 * from the unit tests.
 */
#ifndef HAL_TIME_H
#define HAL_TIME_H

#include <stdint.h>

/* Monotonic time source in microseconds (never goes backwards). */
uint64_t hal_time_now_us(void);

/* Busy/sleep wait for the given number of microseconds. */
void hal_time_delay_us(uint32_t microseconds);

/* Convenience millisecond delay. */
void hal_time_delay_ms(uint32_t milliseconds);

#endif /* HAL_TIME_H */
