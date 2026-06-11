/*
 * ultrasonic.c -- HC-SR04 driver. Depends only on the HAL, so the unit
 * tests replace hal_gpio.h and hal_time.h with CMock mocks.
 */
#include "ultrasonic.h"
#include "hal_gpio.h"
#include "hal_time.h"
#include <stddef.h>

/* HC-SR04 needs a >=10 us HIGH pulse on Trig to start a measurement. */
#define TRIG_SETTLE_US 2u
#define TRIG_PULSE_US 10u

int ultrasonic_init(ultrasonic_t *sensor, unsigned int trig_pin,
                    unsigned int echo_pin)
{
    if (sensor == NULL)
    {
        return HAL_ERR;
    }

    sensor->trig_pin = trig_pin;
    sensor->echo_pin = echo_pin;

    if (hal_gpio_configure(trig_pin, GPIO_DIR_OUTPUT) != HAL_OK)
    {
        return HAL_ERR;
    }
    if (hal_gpio_configure(echo_pin, GPIO_DIR_INPUT) != HAL_OK)
    {
        return HAL_ERR;
    }

    hal_gpio_write(trig_pin, GPIO_LOW);
    return HAL_OK;
}

/* File-local: fire one ping and return the echo pulse width in microseconds,
 * or ULTRASONIC_TIMEOUT on timeout. Exposed to tests via ultrasonic_measure_mm. */
static int ultrasonic_measure_us(const ultrasonic_t *sensor)
{
    uint64_t wait_start;
    uint64_t echo_start;
    uint64_t echo_end;

    if (sensor == NULL)
    {
        return ULTRASONIC_TIMEOUT;
    }

    /* Send the 10 us trigger pulse. */
    hal_gpio_write(sensor->trig_pin, GPIO_LOW);
    hal_time_delay_us(TRIG_SETTLE_US);
    hal_gpio_write(sensor->trig_pin, GPIO_HIGH);
    hal_time_delay_us(TRIG_PULSE_US);
    hal_gpio_write(sensor->trig_pin, GPIO_LOW);

    /* Wait for the rising edge of Echo (with timeout). */
    wait_start = hal_time_now_us();
    while (hal_gpio_read(sensor->echo_pin) == GPIO_LOW)
    {
        if ((hal_time_now_us() - wait_start) > ULTRASONIC_TIMEOUT_US)
        {
            return ULTRASONIC_TIMEOUT;
        }
    }
    echo_start = hal_time_now_us();

    /* Wait for the falling edge of Echo (with timeout). */
    while (hal_gpio_read(sensor->echo_pin) == GPIO_HIGH)
    {
        if ((hal_time_now_us() - wait_start) > ULTRASONIC_TIMEOUT_US)
        {
            return ULTRASONIC_TIMEOUT;
        }
    }
    echo_end = hal_time_now_us();

    return (int)(echo_end - echo_start);
}

/* File-local: convert an echo pulse width (us) to a distance (mm) using the
 * speed of sound (~343 m/s, round trip halved). Negative input passes through. */
static int ultrasonic_us_to_mm(int pulse_us)
{
    if (pulse_us < 0)
    {
        return pulse_us;
    }

    /*
     * distance = (pulse_us * speed_of_sound) / 2
     * speed of sound ~= 0.343 mm/us  ->  mm = pulse_us * 343 / 2000
     */
    return (int)(((long)pulse_us * 343L) / 2000L);
}

int ultrasonic_measure_mm(const ultrasonic_t *sensor)
{
    int pulse_us = ultrasonic_measure_us(sensor);
    return ultrasonic_us_to_mm(pulse_us);
}
