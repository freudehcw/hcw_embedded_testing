/*
 * test_buzzer.c -- tests for the KY-006 passive buzzer driver.
 * hal_gpio.h and hal_time.h are mocked with CMock.
 */
#include "buzzer.h"
#include "mock_hal_gpio.h"
#include "mock_hal_time.h"
#include "unity.h"
#include <stddef.h>

#define BUZZER 22u

void setUp(void)
{
    mock_hal_gpio_Init();
    mock_hal_time_Init();
}

void tearDown(void)
{
    mock_hal_gpio_Verify();
    mock_hal_time_Verify();
    mock_hal_gpio_Destroy();
    mock_hal_time_Destroy();
}

/* ---- buzzer_init -------------------------------------------------------- */

void test_init_null_returns_error(void)
{
    TEST_ASSERT_EQUAL_INT(HAL_ERR, buzzer_init(NULL, BUZZER));
}

void test_init_configure_fails(void)
{
    buzzer_t b;
    hal_gpio_configure_ExpectAndReturn(BUZZER, GPIO_DIR_OUTPUT, HAL_ERR);
    TEST_ASSERT_EQUAL_INT(HAL_ERR, buzzer_init(&b, BUZZER));
}

void test_init_success(void)
{
    buzzer_t b;
    hal_gpio_configure_ExpectAndReturn(BUZZER, GPIO_DIR_OUTPUT, HAL_OK);
    hal_gpio_write_Expect(BUZZER, GPIO_LOW);

    TEST_ASSERT_EQUAL_INT(HAL_OK, buzzer_init(&b, BUZZER));
    TEST_ASSERT_EQUAL_UINT(BUZZER, b.pin);
}

/* ---- buzzer_off --------------------------------------------------------- */

void test_off_null_does_nothing(void)
{
    /* No mock expectations -> if any HAL call happened, Verify would fail. */
    buzzer_off(NULL);
}

void test_off_drives_pin_low(void)
{
    buzzer_t b = {BUZZER};
    hal_gpio_write_Expect(BUZZER, GPIO_LOW);
    buzzer_off(&b);
}

/* ---- buzzer_play_tone ---------------------------------------------------
 * The square-wave maths (file-local buzzer_half_period_us / buzzer_cycle_count)
 * is verified here through the public play_tone: a rest, a sub-cycle duration,
 * and a real tone together exercise every branch of both helpers.
 */

void test_play_tone_null_does_nothing(void)
{
    buzzer_play_tone(NULL, 440, 100);
}

void test_play_tone_rest_is_silent_delay(void)
{
    /* freq <= 0 -> silent rest: pin low, then wait the whole duration. */
    buzzer_t b = {BUZZER};
    hal_gpio_write_Expect(BUZZER, GPIO_LOW);
    hal_time_delay_ms_Expect(30u);
    buzzer_play_tone(&b, 0, 30);
}

void test_play_tone_zero_cycles_no_delay(void)
{
    /* freq>0 but duration too short for a single cycle -> rest, no delay. */
    buzzer_t b = {BUZZER};
    hal_gpio_write_Expect(BUZZER, GPIO_LOW);
    buzzer_play_tone(&b, 100, 0);
}

void test_play_tone_emits_square_wave(void)
{
    /* 1000 Hz for 2 ms -> 2 cycles, half period 500 us. */
    buzzer_t b = {BUZZER};
    int i;
    for (i = 0; i < 2; i++)
    {
        hal_gpio_write_Expect(BUZZER, GPIO_HIGH);
        hal_time_delay_us_Expect(500u);
        hal_gpio_write_Expect(BUZZER, GPIO_LOW);
        hal_time_delay_us_Expect(500u);
    }
    buzzer_play_tone(&b, 1000, 2);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_null_returns_error);
    RUN_TEST(test_init_configure_fails);
    RUN_TEST(test_init_success);

    RUN_TEST(test_off_null_does_nothing);
    RUN_TEST(test_off_drives_pin_low);

    RUN_TEST(test_play_tone_null_does_nothing);
    RUN_TEST(test_play_tone_rest_is_silent_delay);
    RUN_TEST(test_play_tone_zero_cycles_no_delay);
    RUN_TEST(test_play_tone_emits_square_wave);

    return UNITY_END();
}
