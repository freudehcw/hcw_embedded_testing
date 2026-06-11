/*
 * test_ultrasonic.c -- tests for the HC-SR04 driver.
 *
 * hal_gpio.h and hal_time.h are replaced by CMock-generated mocks, so we can
 * script exact GPIO/time sequences and assert the driver reacts correctly --
 * no real sensor required.
 */
#include "mock_hal_gpio.h"
#include "mock_hal_time.h"
#include "ultrasonic.h"
#include "unity.h"
#include <stddef.h>

#define TRIG 17u
#define ECHO 27u

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

/* ---- ultrasonic_init ---------------------------------------------------- */

void test_init_null_returns_error(void)
{
    TEST_ASSERT_EQUAL_INT(HAL_ERR, ultrasonic_init(NULL, TRIG, ECHO));
}

void test_init_trig_configure_fails(void)
{
    ultrasonic_t s;
    hal_gpio_configure_ExpectAndReturn(TRIG, GPIO_DIR_OUTPUT, HAL_ERR);
    TEST_ASSERT_EQUAL_INT(HAL_ERR, ultrasonic_init(&s, TRIG, ECHO));
}

void test_init_echo_configure_fails(void)
{
    ultrasonic_t s;
    hal_gpio_configure_ExpectAndReturn(TRIG, GPIO_DIR_OUTPUT, HAL_OK);
    hal_gpio_configure_ExpectAndReturn(ECHO, GPIO_DIR_INPUT, HAL_ERR);
    TEST_ASSERT_EQUAL_INT(HAL_ERR, ultrasonic_init(&s, TRIG, ECHO));
}

void test_init_success(void)
{
    ultrasonic_t s;
    hal_gpio_configure_ExpectAndReturn(TRIG, GPIO_DIR_OUTPUT, HAL_OK);
    hal_gpio_configure_ExpectAndReturn(ECHO, GPIO_DIR_INPUT, HAL_OK);
    hal_gpio_write_Expect(TRIG, GPIO_LOW);

    TEST_ASSERT_EQUAL_INT(HAL_OK, ultrasonic_init(&s, TRIG, ECHO));
    TEST_ASSERT_EQUAL_UINT(TRIG, s.trig_pin);
    TEST_ASSERT_EQUAL_UINT(ECHO, s.echo_pin);
}

/* Helper: queue the fixed 10 us trigger pulse the driver always emits. */
static void expect_trigger_pulse(void)
{
    hal_gpio_write_Expect(TRIG, GPIO_LOW);
    hal_time_delay_us_Expect(2u);
    hal_gpio_write_Expect(TRIG, GPIO_HIGH);
    hal_time_delay_us_Expect(10u);
    hal_gpio_write_Expect(TRIG, GPIO_LOW);
}

/* ---- ultrasonic_measure_mm ---------------------------------------------
 * These drive the file-local measure_us / us_to_mm helpers through the
 * public measure_mm entry point, covering every branch (trigger, both echo
 * edges, both timeouts, the us->mm conversion and the error passthrough).
 */

void test_measure_mm_null_returns_timeout(void)
{
    /* NULL -> measure_us NULL guard -> timeout -> us_to_mm passes it through. */
    TEST_ASSERT_EQUAL_INT(ULTRASONIC_TIMEOUT, ultrasonic_measure_mm(NULL));
}

void test_measure_mm_happy_path(void)
{
    ultrasonic_t s = {TRIG, ECHO};

    expect_trigger_pulse();
    hal_time_now_us_ExpectAndReturn(1000u);   /* wait_start */
    /* rising edge: one LOW poll, then HIGH */
    hal_gpio_read_ExpectAndReturn(ECHO, GPIO_LOW);
    hal_time_now_us_ExpectAndReturn(1005u);   /* timeout check, ok */
    hal_gpio_read_ExpectAndReturn(ECHO, GPIO_HIGH);
    hal_time_now_us_ExpectAndReturn(1010u);   /* echo_start */
    /* falling edge: one HIGH poll, then LOW */
    hal_gpio_read_ExpectAndReturn(ECHO, GPIO_HIGH);
    hal_time_now_us_ExpectAndReturn(1015u);   /* timeout check, ok */
    hal_gpio_read_ExpectAndReturn(ECHO, GPIO_LOW);
    hal_time_now_us_ExpectAndReturn(1510u);   /* echo_end -> 500 us */

    /* 500 us -> 500 * 343 / 2000 = 85 mm */
    TEST_ASSERT_EQUAL_INT(85, ultrasonic_measure_mm(&s));
}

void test_measure_mm_timeout_waiting_for_rising_edge(void)
{
    ultrasonic_t s = {TRIG, ECHO};

    expect_trigger_pulse();
    hal_time_now_us_ExpectAndReturn(1000u);                 /* wait_start */
    hal_gpio_read_ExpectAndReturn(ECHO, GPIO_LOW);          /* still low */
    hal_time_now_us_ExpectAndReturn(1000u + ULTRASONIC_TIMEOUT_US + 1u);

    TEST_ASSERT_EQUAL_INT(ULTRASONIC_TIMEOUT, ultrasonic_measure_mm(&s));
}

void test_measure_mm_timeout_waiting_for_falling_edge(void)
{
    ultrasonic_t s = {TRIG, ECHO};

    expect_trigger_pulse();
    hal_time_now_us_ExpectAndReturn(1000u);         /* wait_start */
    hal_gpio_read_ExpectAndReturn(ECHO, GPIO_HIGH); /* rising edge already up */
    hal_time_now_us_ExpectAndReturn(1010u);         /* echo_start */
    hal_gpio_read_ExpectAndReturn(ECHO, GPIO_HIGH); /* never falls */
    hal_time_now_us_ExpectAndReturn(1000u + ULTRASONIC_TIMEOUT_US + 2u);

    TEST_ASSERT_EQUAL_INT(ULTRASONIC_TIMEOUT, ultrasonic_measure_mm(&s));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_null_returns_error);
    RUN_TEST(test_init_trig_configure_fails);
    RUN_TEST(test_init_echo_configure_fails);
    RUN_TEST(test_init_success);

    RUN_TEST(test_measure_mm_null_returns_timeout);
    RUN_TEST(test_measure_mm_happy_path);
    RUN_TEST(test_measure_mm_timeout_waiting_for_rising_edge);
    RUN_TEST(test_measure_mm_timeout_waiting_for_falling_edge);

    return UNITY_END();
}
