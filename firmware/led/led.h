#ifndef LED_H_
#define LED_H_

#include "color.h"

typedef enum {
    LED_1,
    LED_INVALID,
} led_t;

typedef void *led_handle_t;

led_handle_t led_init(led_t led);

void led_color_set(led_handle_t handle, const color_t *color);

#endif /*LED_H_*/