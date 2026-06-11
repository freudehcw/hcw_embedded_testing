/*
 * hal_gpio.h -- Hardware Abstraction Layer for GPIO.
 *
 * This header is the seam between the testable application logic and the
 * real hardware. In the host unit-test build CMock generates a mock from
 * exactly this header, so the application code (ultrasonic.c, buzzer.c)
 * can be tested without any Raspberry Pi present. The real implementation
 * lives in src/hal_gpio_pi.c and is only compiled into the target binary.
 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

typedef enum
{
    GPIO_LOW = 0,
    GPIO_HIGH = 1
} gpio_level_t;

typedef enum
{
    GPIO_DIR_INPUT = 0,
    GPIO_DIR_OUTPUT = 1
} gpio_dir_t;

/* Return codes used across the HAL. */
#define HAL_OK 0
#define HAL_ERR (-1)

/*
 * Open the GPIO backend (the gpiochip character device on the Pi).
 * Returns HAL_OK on success, HAL_ERR otherwise.
 */
int hal_gpio_init(void);

/* Release every claimed line and close the backend. */
void hal_gpio_close(void);

/*
 * Claim a single line and set its direction.
 * Returns HAL_OK on success, HAL_ERR otherwise.
 */
int hal_gpio_configure(unsigned int pin, gpio_dir_t dir);

/* Drive an output line. */
void hal_gpio_write(unsigned int pin, gpio_level_t level);

/* Sample an input line. */
gpio_level_t hal_gpio_read(unsigned int pin);

#endif /* HAL_GPIO_H */
