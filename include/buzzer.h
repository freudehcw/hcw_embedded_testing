/*
 * buzzer.h -- KY-006 passive piezo buzzer driver.
 *
 * Wiring on the Raspberry Pi 5:
 *   KY-006 Signal -> Pin 15 (GPIO 22)
 *   KY-006 GND    -> Pin 9  (GND)
 *   (the KY-006 module is driven straight from the GPIO; a passive buzzer
 *    needs a square wave at the desired pitch -- we bit-bang it via the HAL.)
 *
 * Because a passive buzzer makes no sound on its own, the driver toggles the
 * signal pin at the tone frequency. The maths is split into pure helpers so
 * the timing can be verified without counting thousands of mock calls.
 */
#ifndef BUZZER_H
#define BUZZER_H

typedef struct
{
    unsigned int pin;
} buzzer_t;

/* Configure the buzzer pin as output and leave it silent.
 * Returns HAL_OK on success or HAL_ERR. */
int buzzer_init(buzzer_t *buzzer, unsigned int pin);

/* Force the buzzer pin low (no sound). */
void buzzer_off(const buzzer_t *buzzer);

/*
 * Play a tone of freq_hz for duration_ms by bit-banging the pin.
 * freq_hz <= 0 is treated as a silent rest of the given duration.
 *
 * (The square-wave maths -- half period and cycle count -- lives in
 * file-local static helpers in buzzer.c and is verified through this
 * function in the unit tests.)
 */
void buzzer_play_tone(const buzzer_t *buzzer, int freq_hz, int duration_ms);

#endif /* BUZZER_H */
