/*
 * buzzer.c -- KY-006 passive buzzer driver. Depends only on the HAL.
 */
#include "buzzer.h"
#include "hal_gpio.h"
#include "hal_time.h"
#include <stddef.h>

int buzzer_init(buzzer_t *buzzer, unsigned int pin)
{
    if (buzzer == NULL)
    {
        return HAL_ERR;
    }

    buzzer->pin = pin;

    if (hal_gpio_configure(pin, GPIO_DIR_OUTPUT) != HAL_OK)
    {
        return HAL_ERR;
    }

    hal_gpio_write(pin, GPIO_LOW);
    return HAL_OK;
}

void buzzer_off(const buzzer_t *buzzer)
{
    if (buzzer == NULL)
    {
        return;
    }
    hal_gpio_write(buzzer->pin, GPIO_LOW);
}

static unsigned int buzzer_half_period_us(int freq_hz)
{
    if (freq_hz <= 0)
    {
        return 0u;
    }
    /* full period = 1_000_000 / f us; half of that toggles the pin. */
    return (unsigned int)(1000000L / (2L * (long)freq_hz));
}

static unsigned int buzzer_cycle_count(int freq_hz, int duration_ms)
{
    if (freq_hz <= 0 || duration_ms <= 0)
    {
        return 0u;
    }
    return (unsigned int)(((long)freq_hz * (long)duration_ms) / 1000L);
}

void buzzer_play_tone(const buzzer_t *buzzer, int freq_hz, int duration_ms)
{
    unsigned int half;
    unsigned int cycles;
    unsigned int i;

    if (buzzer == NULL)
    {
        return;
    }

    half = buzzer_half_period_us(freq_hz);
    cycles = buzzer_cycle_count(freq_hz, duration_ms);

    /* A rest (or an unplayably low frequency): stay silent for the duration. */
    if (half == 0u || cycles == 0u)
    {
        hal_gpio_write(buzzer->pin, GPIO_LOW);
        if (duration_ms > 0)
        {
            hal_time_delay_ms((uint32_t)duration_ms);
        }
        return;
    }

    for (i = 0u; i < cycles; i++)
    {
        hal_gpio_write(buzzer->pin, GPIO_HIGH);
        hal_time_delay_us(half);
        hal_gpio_write(buzzer->pin, GPIO_LOW);
        hal_time_delay_us(half);
    }
}
