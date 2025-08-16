#include "led.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(LED, CONFIG_AWS_IOT_SAMPLE_LOG_LEVEL);

typedef struct {
    struct pwm_dt_spec pwm_r;
    struct pwm_dt_spec pwm_g;
    struct pwm_dt_spec pwm_b;
    int period; // nanosecs
} led_ctx_t;

static led_ctx_t rgb_led = {
    .pwm_r = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_led0)),
    .pwm_g = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_led2)),
    .pwm_b = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_led1)),
    .period = 8 * 1000 * 1000
};

led_handle_t led_init(led_t led)
{
    if (led == LED_1)
    {
        bool ready = pwm_is_ready_dt(&rgb_led.pwm_r);
	    ready &= pwm_is_ready_dt(&rgb_led.pwm_g);
	    ready &= pwm_is_ready_dt(&rgb_led.pwm_b);

        if (ready)
        {
            return (led_handle_t)&(rgb_led);
        }
    }

    return NULL;
}

void led_color_set(led_handle_t handle, const color_t *color)
{
    led_ctx_t *ctx = (led_ctx_t *)handle;
    LOG_INF("Changing color");
    pwm_set_pulse_dt(&ctx->pwm_r, (int)(ctx->period * color->r));
    pwm_set_pulse_dt(&ctx->pwm_g, (int)(ctx->period * color->g));
    pwm_set_pulse_dt(&ctx->pwm_b, (int)(ctx->period * color->b));
    LOG_INF("Color changed");
}