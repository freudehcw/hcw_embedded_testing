/*
 * ultrasonic.h -- HC-SR04 ultrasonic distance sensor driver.
 *
 * Wiring on the Raspberry Pi 5:
 *   SR04 VCC  -> Pin 1  (3.3V)
 *   SR04 GND  -> Pin 6  (GND)
 *   SR04 Trig -> Pin 11 (GPIO 17)
 *   SR04 Echo -> Pin 13 (GPIO 27)   (use a level divider, Echo is 5V!)
 *
 * The driver only talks to the HAL, so it is fully unit-testable with mocks.
 */
#ifndef ULTRASONIC_H
#define ULTRASONIC_H

/* Returned by the measure functions when the echo never arrives. */
#define ULTRASONIC_TIMEOUT (-1)

/* Maximum time (us) we wait for the echo edges before giving up.
 * 30000 us round trip ~= 5 m, well beyond the HC-SR04 range. */
#define ULTRASONIC_TIMEOUT_US 30000u

typedef struct
{
    unsigned int trig_pin;
    unsigned int echo_pin;
} ultrasonic_t;

/*
 * Configure the trigger pin as output and the echo pin as input.
 * Returns HAL_OK on success or HAL_ERR if a pin could not be configured.
 */
int ultrasonic_init(ultrasonic_t *sensor, unsigned int trig_pin,
                    unsigned int echo_pin);

/*
 * Fire one ping and return the measured distance in millimetres,
 * or ULTRASONIC_TIMEOUT on timeout.
 *
 * (The internal steps -- measuring the echo pulse width in microseconds and
 * converting it to millimetres -- are file-local static helpers in
 * ultrasonic.c and are exercised through this function in the unit tests.)
 */
int ultrasonic_measure_mm(const ultrasonic_t *sensor);

#endif /* ULTRASONIC_H */
