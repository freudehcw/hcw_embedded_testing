/*
 * test_theremin.c -- tests for the pure pitch-mapping logic.
 * No mocks needed: theremin.c has no HAL dependency.
 */
#include "theremin.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* ---- theremin_distance_to_freq ------------------------------------------ */

void test_distance_negative_is_silent(void)
{
    TEST_ASSERT_EQUAL_INT(THEREMIN_SILENCE, theremin_distance_to_freq(-1));
}

void test_distance_beyond_max_is_silent(void)
{
    TEST_ASSERT_EQUAL_INT(THEREMIN_SILENCE,
                          theremin_distance_to_freq(THEREMIN_MAX_MM + 1));
}

void test_distance_below_min_clamps_to_highest_pitch(void)
{
    /* Anything closer than MIN should give the top frequency. */
    TEST_ASSERT_EQUAL_INT(THEREMIN_MAX_HZ, theremin_distance_to_freq(10));
}

void test_distance_at_min_is_max_hz(void)
{
    TEST_ASSERT_EQUAL_INT(THEREMIN_MAX_HZ,
                          theremin_distance_to_freq(THEREMIN_MIN_MM));
}

void test_distance_at_max_is_min_hz(void)
{
    TEST_ASSERT_EQUAL_INT(THEREMIN_MIN_HZ,
                          theremin_distance_to_freq(THEREMIN_MAX_MM));
}

void test_distance_midpoint_is_middle_pitch(void)
{
    /* Midpoint distance (225 mm) -> midpoint frequency (550 Hz). */
    int mid_mm = (THEREMIN_MIN_MM + THEREMIN_MAX_MM) / 2;
    int expected = (THEREMIN_MIN_HZ + THEREMIN_MAX_HZ) / 2;
    TEST_ASSERT_EQUAL_INT(expected, theremin_distance_to_freq(mid_mm));
}

/* ---- theremin_snap_to_scale --------------------------------------------- */

void test_snap_silence_passthrough(void)
{
    TEST_ASSERT_EQUAL_INT(THEREMIN_SILENCE,
                          theremin_snap_to_scale(THEREMIN_SILENCE));
}

void test_snap_negative_is_silence(void)
{
    TEST_ASSERT_EQUAL_INT(THEREMIN_SILENCE, theremin_snap_to_scale(-5));
}

void test_snap_exact_note_unchanged(void)
{
    TEST_ASSERT_EQUAL_INT(440, theremin_snap_to_scale(440));
}

void test_snap_rounds_up_to_nearest(void)
{
    /* 225 Hz is closest to 220 (A3). */
    TEST_ASSERT_EQUAL_INT(220, theremin_snap_to_scale(225));
}

void test_snap_picks_best_in_middle(void)
{
    /* 500 Hz is closest to 523 (C5) and forces the loop to update twice. */
    TEST_ASSERT_EQUAL_INT(523, theremin_snap_to_scale(500));
}

void test_snap_below_range_clamps_to_bottom(void)
{
    /* 100 Hz is below the lowest note (220) -> initial abs() branch. */
    TEST_ASSERT_EQUAL_INT(220, theremin_snap_to_scale(100));
}

void test_snap_above_range_clamps_to_top(void)
{
    TEST_ASSERT_EQUAL_INT(880, theremin_snap_to_scale(2000));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_distance_negative_is_silent);
    RUN_TEST(test_distance_beyond_max_is_silent);
    RUN_TEST(test_distance_below_min_clamps_to_highest_pitch);
    RUN_TEST(test_distance_at_min_is_max_hz);
    RUN_TEST(test_distance_at_max_is_min_hz);
    RUN_TEST(test_distance_midpoint_is_middle_pitch);

    RUN_TEST(test_snap_silence_passthrough);
    RUN_TEST(test_snap_negative_is_silence);
    RUN_TEST(test_snap_exact_note_unchanged);
    RUN_TEST(test_snap_rounds_up_to_nearest);
    RUN_TEST(test_snap_picks_best_in_middle);
    RUN_TEST(test_snap_below_range_clamps_to_bottom);
    RUN_TEST(test_snap_above_range_clamps_to_top);

    return UNITY_END();
}
