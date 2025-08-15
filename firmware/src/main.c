/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include <stdio.h>
#include <stdlib.h>
#include <modem/modem_info.h>

#include "error.h"
#include "led.h"
#include "mqtt_client.h"


/* Register log module */
LOG_MODULE_REGISTER(aws_iot_sample, CONFIG_AWS_IOT_SAMPLE_LOG_LEVEL);

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

int color_from_glucose(int g, color_t *c);
void on_glucose(int glucose);

led_handle_t led = NULL;

static void on_net_event_l4_connected(void)
{
	mqtt_appclient_connection_start();
}

static void on_net_event_l4_disconnected(void)
{
	mqtt_appclient_connection_stop();
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		on_net_event_l4_connected();
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		on_net_event_l4_disconnected();
		break;
	default:
		/* Don't care */
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

int main(void)
{
	LOG_INF("The AWS IoT sample started, version: %s", CONFIG_AWS_IOT_SAMPLE_APP_VERSION);

	int err;

	led = led_init(LED_1);
	led_color_set(led, &COLOR_WHITE);

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	err = mqtt_appclient_init();
	if (err) {
		LOG_ERR("mqtt_client_init, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	mqtt_appclient_reg_glucose_cb(on_glucose);

	return 0;
}

int color_from_glucose(int g, color_t *c) 
{
    if (g < 0)
	{
		return -1;
	}       
	else if (g < 55)
	{
		memcpy(c, &COLOR_RED, sizeof(COLOR_RED));
	}
	else if (g < 152)
	{
		color_interp(
			&COLOR_RED, 
			&COLOR_GREEN, 
			((float)(g-55.0f))/(152.0f-55.0f), 
			c
		);
	}
	else if (g < 250)
	{
		color_interp(
			&COLOR_GREEN, 
			&COLOR_BLUE, 
			((float)(g-152.0f))/(250.0f-152.0f), 
			c
		);
	}
	else if (g < 300)
	{
		color_interp(
			&COLOR_BLUE, 
			&COLOR_PURPLE, 
			((float)(g-250.0f))/(300.0f-250.0f), 
			c
		);
	}
	else
	{
		memcpy(c, &COLOR_PURPLE, sizeof(COLOR_PURPLE));
	}

	return 0;
}

void on_glucose(int glucose)
{
	color_t c;
	if (led != NULL)
	{
		LOG_INF("Setting LED color");
		color_from_glucose(glucose, &c);
		led_color_set(led, &c);
	}
}