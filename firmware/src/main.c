/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "led.h"
#include "mqtt_client.h"
#include "network.h"

LOG_MODULE_REGISTER(sugarcube, CONFIG_AWS_IOT_SAMPLE_LOG_LEVEL);

// Blood glucose helpers to convert bg to color
#define BG_SCALE(g, low, high) (((float)(g - low))/(high - low))

// Map between BG and LED color
#define BG_VLOW_VALUE	55
#define BG_VLOW_COLOR	COLOR_RED

#define BG_LOW_VALUE 	152
#define BG_LOW_COLOR	COLOR_GREEN

#define BG_MED_VALUE 	250
#define BG_MED_COLOR    COLOR_BLUE

#define BG_HIGH_VALUE	300
#define BG_HIGH_COLOR	COLOR_PURPLE


static int bg_to_color(int g, color_t *c);
void test_led(led_handle_t led);

// Callbacks
static void on_glucose(int glucose);
static void net_evt_cb(net_evt_t evt, void *ctx);

// LED handle
static led_handle_t led = NULL;

int main(void)
{
	int err;

	led = led_init(LED_1);
	test_led(led);
	led_color_set(led, &COLOR_WHITE);

	// Enable connectivity
	LOG_INF("Bringing network interface up and connecting to the network");
	network_cb_reg(net_evt_cb);
	network_init();

	// Connect to AWS IOT server
	err = mqtt_appclient_init();
	if (err) {
		LOG_ERR("mqtt_client_init, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	mqtt_appclient_reg_glucose_cb(on_glucose);

	return 0;
}

void net_evt_cb(net_evt_t evt, void *ctx)
{
	switch (evt)
	{
		case NET_EVT_CONNECTED:
			mqtt_appclient_connection_start();
			break;

		case NET_EVT_DISCONNECTED:
			mqtt_appclient_connection_stop();
			break;
	}
}

void on_glucose(int glucose)
{
	color_t c;
	if (led != NULL)
	{
		bg_to_color(glucose, &c);
		led_color_set(led, &c);
	}
}

int bg_to_color(int g, color_t *c) 
{
    if (g < 0)
	{
		// Invalid glucose
		return -1;
	}       
	else if (g < BG_VLOW_VALUE)
	{
		// Very low
		memcpy(c, &BG_VLOW_COLOR, sizeof(BG_VLOW_COLOR));
	}
	else if (g < BG_LOW_VALUE)
	{
		// Low
		color_interp(
			&BG_VLOW_COLOR, &BG_LOW_COLOR,
			BG_SCALE(g, BG_VLOW_VALUE, BG_LOW_VALUE),
			c
		);
	}
	else if (g < BG_MED_VALUE)
	{
		// Medium
		color_interp(
			&BG_LOW_COLOR, &BG_MED_COLOR, 
			BG_SCALE(g, BG_LOW_VALUE, BG_MED_VALUE),
			c
		);
	}
	else if (g < BG_HIGH_VALUE)
	{
		// High
		color_interp(
			&BG_MED_COLOR, &BG_HIGH_COLOR, 
			BG_SCALE(g, BG_MED_VALUE, BG_HIGH_VALUE),
			c
		);
	}
	else
	{
		// Very high
		memcpy(c, &BG_HIGH_COLOR, sizeof(BG_HIGH_COLOR));
	}

	return 0;
}

void test_led(led_handle_t led)
{
	color_t c;
	for (int g=0; g<350; g+=10)
	{
		bg_to_color(g, &c);
		led_color_set(led, &c);
		k_msleep(100);
	}
}