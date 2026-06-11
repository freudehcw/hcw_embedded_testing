/*
 * theremin.h -- Pure theremin logic: map a hand distance to a pitch.
 *
 * No hardware, no HAL: every function here is a deterministic pure function,
 * which makes them trivial to unit-test and great for coverage.
 *
 * Playing model:
 *   - Hand closer than MIN  -> clamped to the highest pitch.
 *   - Hand farther than MAX -> silence (you "lift off" the instrument).
 *   - In between            -> closer = higher pitch (linear interpolation).
 */
#ifndef THEREMIN_H
#define THEREMIN_H

#define THEREMIN_MIN_MM 50   /* closest playable distance */
#define THEREMIN_MAX_MM 400  /* farthest playable distance */
#define THEREMIN_MIN_HZ 220  /* A3, played at THEREMIN_MAX_MM */
#define THEREMIN_MAX_HZ 880  /* A5, played at THEREMIN_MIN_MM */
#define THEREMIN_SILENCE 0   /* returned when no pitch should sound */

/*
 * Map a distance in millimetres to a frequency in Hz.
 * Negative distance (sensor error/timeout) -> THEREMIN_SILENCE.
 */
int theremin_distance_to_freq(int distance_mm);

/*
 * Snap an arbitrary frequency to the nearest note of an A-minor pentatonic
 * scale (A3..A5). This gives the instrument a musical, "in-tune" feel.
 * THEREMIN_SILENCE is passed through unchanged.
 */
int theremin_snap_to_scale(int freq_hz);

#endif /* THEREMIN_H */
