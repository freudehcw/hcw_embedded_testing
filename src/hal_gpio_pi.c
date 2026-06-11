/*
 * hal_gpio_pi.c -- real GPIO HAL for the Raspberry Pi 5.
 *
 * Uses the Linux GPIO v2 character-device interface (<linux/gpio.h>),
 * the same ABI that libgpiod wraps. This needs NO external library, so it
 * cross-compiles with just the toolchain. It is compiled only into the
 * target binary and is therefore intentionally excluded from coverage --
 * it is the untestable hardware boundary that the mocks stand in for.
 *
 * On the Pi 5 the 40-pin header lives on a gpiochip provided by the RP1.
 * Depending on the kernel that is "gpiochip0" or "gpiochip4"; override with
 *   make ... -DHAL_GPIOCHIP='"/dev/gpiochip4"'
 * or edit HAL_GPIOCHIP below if your kernel names it differently
 * (check with: gpiodetect).
 */
#include "hal_gpio.h"

#include <fcntl.h>
#include <linux/gpio.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifndef HAL_GPIOCHIP
#define HAL_GPIOCHIP "/dev/gpiochip0"
#endif

/* The Pi header has 28 user GPIOs (0..27); we keep one request fd per line. */
#define HAL_MAX_LINES 28

static int s_chip_fd = -1;
static int s_line_fd[HAL_MAX_LINES];

int hal_gpio_init(void)
{
    int i;
    for (i = 0; i < HAL_MAX_LINES; i++)
    {
        s_line_fd[i] = -1;
    }

    s_chip_fd = open(HAL_GPIOCHIP, O_RDWR | O_CLOEXEC);
    if (s_chip_fd < 0)
    {
        perror("hal_gpio_init: open " HAL_GPIOCHIP);
        return HAL_ERR;
    }
    return HAL_OK;
}

void hal_gpio_close(void)
{
    int i;
    for (i = 0; i < HAL_MAX_LINES; i++)
    {
        if (s_line_fd[i] >= 0)
        {
            close(s_line_fd[i]);
            s_line_fd[i] = -1;
        }
    }
    if (s_chip_fd >= 0)
    {
        close(s_chip_fd);
        s_chip_fd = -1;
    }
}

int hal_gpio_configure(unsigned int pin, gpio_dir_t dir)
{
    struct gpio_v2_line_request req;

    if (pin >= HAL_MAX_LINES || s_chip_fd < 0)
    {
        return HAL_ERR;
    }

    if (s_line_fd[pin] >= 0)
    {
        close(s_line_fd[pin]);
        s_line_fd[pin] = -1;
    }

    memset(&req, 0, sizeof(req));
    req.num_lines = 1;
    req.offsets[0] = pin;
    req.config.flags = (dir == GPIO_DIR_OUTPUT)
                           ? GPIO_V2_LINE_FLAG_OUTPUT
                           : GPIO_V2_LINE_FLAG_INPUT;
    strncpy(req.consumer, "theremin", sizeof(req.consumer) - 1);

    if (ioctl(s_chip_fd, GPIO_V2_GET_LINE_IOCTL, &req) < 0)
    {
        perror("hal_gpio_configure: GPIO_V2_GET_LINE_IOCTL");
        return HAL_ERR;
    }

    s_line_fd[pin] = req.fd;
    return HAL_OK;
}

void hal_gpio_write(unsigned int pin, gpio_level_t level)
{
    struct gpio_v2_line_values vals;

    if (pin >= HAL_MAX_LINES || s_line_fd[pin] < 0)
    {
        return;
    }

    memset(&vals, 0, sizeof(vals));
    vals.mask = 1u;
    vals.bits = (level == GPIO_HIGH) ? 1u : 0u;

    if (ioctl(s_line_fd[pin], GPIO_V2_LINE_SET_VALUES_IOCTL, &vals) < 0)
    {
        perror("hal_gpio_write: GPIO_V2_LINE_SET_VALUES_IOCTL");
    }
}

gpio_level_t hal_gpio_read(unsigned int pin)
{
    struct gpio_v2_line_values vals;

    if (pin >= HAL_MAX_LINES || s_line_fd[pin] < 0)
    {
        return GPIO_LOW;
    }

    memset(&vals, 0, sizeof(vals));
    vals.mask = 1u;

    if (ioctl(s_line_fd[pin], GPIO_V2_LINE_GET_VALUES_IOCTL, &vals) < 0)
    {
        return GPIO_LOW;
    }

    return (vals.bits & 1u) ? GPIO_HIGH : GPIO_LOW;
}
