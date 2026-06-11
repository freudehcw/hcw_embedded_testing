/*
 * theremin.c -- pure pitch-mapping logic. No HAL dependency.
 */
#include "theremin.h"

/* A-minor pentatonic across two octaves (A3, C4, D4, E4, G4, A4, C5, D5,
 * E5, G5, A5), rounded to whole Hz. Used to "auto-tune" the theremin. */
static const int kScale[] = {
    220, 262, 294, 330, 392,
    440, 523, 587, 659, 784, 880};

#define SCALE_LEN ((int)(sizeof(kScale) / sizeof(kScale[0])))

/* Absolute value of an int (kept tiny and separate so the nearest-note search
 * below stays a single, simple branch per iteration). */
static int abs_int(int value)
{
    return (value < 0) ? -value : value;
}

int theremin_distance_to_freq(int distance_mm)
{
    int clamped;
    long span_mm;
    long span_hz;
    long offset;
    int freq;

    /* Error / no hand in range -> silence. */
    if (distance_mm < 0 || distance_mm > THEREMIN_MAX_MM)
    {
        return THEREMIN_SILENCE;
    }

    /* Too close: clamp to the nearest playable distance (highest pitch). */
    clamped = distance_mm;
    if (clamped < THEREMIN_MIN_MM)
    {
        clamped = THEREMIN_MIN_MM;
    }

    /*
     * Linear interpolation. ratio = (clamped - MIN) / (MAX - MIN) is 0 when
     * the hand is nearest and 1 when farthest. Near -> MAX_HZ, far -> MIN_HZ:
     *   freq = MAX_HZ - ratio * (MAX_HZ - MIN_HZ)
     */
    span_mm = (long)THEREMIN_MAX_MM - (long)THEREMIN_MIN_MM;
    span_hz = (long)THEREMIN_MAX_HZ - (long)THEREMIN_MIN_HZ;
    offset = ((long)(clamped - THEREMIN_MIN_MM) * span_hz) / span_mm;
    freq = (int)((long)THEREMIN_MAX_HZ - offset);

    return freq;
}

int theremin_snap_to_scale(int freq_hz)
{
    int i;
    int best;
    int best_dist;

    if (freq_hz <= THEREMIN_SILENCE)
    {
        return THEREMIN_SILENCE;
    }

    best = kScale[0];
    best_dist = abs_int(freq_hz - kScale[0]);

    for (i = 1; i < SCALE_LEN; i++)
    {
        int dist = abs_int(freq_hz - kScale[i]);
        if (dist < best_dist)
        {
            best_dist = dist;
            best = kScale[i];
        }
    }

    return best;
}
