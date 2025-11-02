#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGBColor;

int rgb_init(gpio_num_t gpio_num);
void rgb_set(bool state);
void rgb_toggle();
void rgb_set_color(RGBColor color);