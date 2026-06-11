/*
 * main.c -- the actual theremin. Composition root + main loop.
 *
 * This file just wires the tested modules to the real HAL and loops; it is
 * the untestable "edge" of the program (infinite loop, hardware init) and is
 * deliberately kept tiny and excluded from coverage.
 *
 *  Wiring (Raspberry Pi 5, 40-pin header):
 *    SR04  VCC  -> Pin 1  (3.3V)
 *    SR04  GND  -> Pin 6  (GND)
 *    SR04  Trig -> Pin 11 (GPIO 17)
 *    SR04  Echo -> Pin 13 (GPIO 27)   ! 5V echo, use a level shifter/divider
 *    KY-006 VCC    -> Pin 2  (5V)
 *    KY-006 GND    -> Pin 9  (GND)
 *    KY-006 Signal -> Pin 15 (GPIO 22)
 */
#include "buzzer.h"
#include "hal_gpio.h"
#include "hal_time.h"
#include "theremin.h"
#include "ultrasonic.h"

#include <signal.h>
#include <stdio.h>

#define TRIG_PIN 17u
#define ECHO_PIN 27u
#define BUZZER_PIN 22u

/* Each loop plays a short tone so the pitch can follow the hand. */
#define NOTE_MS 30

static volatile sig_atomic_t g_running = 1;

static void on_sigint(int sig)
{
    (void)sig;
    g_running = 0;
}

int main(void)
{
    ultrasonic_t sensor;
    buzzer_t buzzer;

    if (signal(SIGINT, on_sigint) == SIG_ERR)
    {
        fprintf(stderr, "could not install signal handler\n");
        return 1;
    }

    if (hal_gpio_init() != HAL_OK)
    {
        fprintf(stderr, "GPIO init failed (run with sufficient privileges?)\n");
        return 1;
    }

    if (ultrasonic_init(&sensor, TRIG_PIN, ECHO_PIN) != HAL_OK ||
        buzzer_init(&buzzer, BUZZER_PIN) != HAL_OK)
    {
        fprintf(stderr, "peripheral init failed\n");
        hal_gpio_close();
        return 1;
    }

    printf("Theremin running. Move your hand over the sensor. Ctrl-C to stop.\n");

    while (g_running)
    {
        int distance_mm = ultrasonic_measure_mm(&sensor);
        int freq = theremin_distance_to_freq(distance_mm);
        freq = theremin_snap_to_scale(freq);

        if (freq == THEREMIN_SILENCE)
        {
            buzzer_off(&buzzer);
            hal_time_delay_ms(NOTE_MS);
        }
        else
        {
            buzzer_play_tone(&buzzer, freq, NOTE_MS);
        }
    }

    buzzer_off(&buzzer);
    hal_gpio_close();
    printf("\nStopped.\n");
    return 0;
}
