#include "mqtt_client.h"
#include "msg.h"
#include "error.h"

#include <net/aws_iot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>

LOG_MODULE_REGISTER(mqtt_client, CONFIG_AWS_IOT_SAMPLE_LOG_LEVEL);

// Application specific topics
#define MY_CUSTOM_TOPIC_1 "glucose/value"

static void aws_iot_event_handler(const struct aws_iot_evt *const evt);
static int app_topics_subscribe(void);

static void on_aws_iot_evt_disconnected(void);
static void on_aws_iot_evt_connected(const struct aws_iot_evt *const evt);

static void connect_work_fn(struct k_work *work);

// Work items used to control some aspects of the sample
static K_WORK_DELAYABLE_DEFINE(connect_work, connect_work_fn);

static glucose_cb_t _cb = NULL;

static void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
	switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING:
		LOG_INF("AWS_IOT_EVT_CONNECTING");
		break;
	case AWS_IOT_EVT_CONNECTED:
		LOG_INF("AWS_IOT_EVT_CONNECTED");
		on_aws_iot_evt_connected(evt);
		break;
	case AWS_IOT_EVT_DISCONNECTED:
		LOG_INF("AWS_IOT_EVT_DISCONNECTED");
		on_aws_iot_evt_disconnected();
		break;
	case AWS_IOT_EVT_DATA_RECEIVED:
		LOG_INF("AWS_IOT_EVT_DATA_RECEIVED");

		LOG_INF("Received message: \"%.*s\" on topic: \"%.*s\"", evt->data.msg.len,
									 evt->data.msg.ptr,
									 evt->data.msg.topic.len,
									 evt->data.msg.topic.str);
		
        int glucose = 0;
			
		int64_t rc = msg_glucose_decode(
			evt->data.msg.ptr,
			evt->data.msg.len, 
			&glucose
		);

		if (rc > 0 && _cb)
		{
            _cb(glucose);
		}

		break;
	case AWS_IOT_EVT_PUBACK:
		LOG_INF("AWS_IOT_EVT_PUBACK, message ID: %d", evt->data.message_id);
		break;
	case AWS_IOT_EVT_PINGRESP:
		LOG_INF("AWS_IOT_EVT_PINGRESP");
		break;
	case AWS_IOT_EVT_ERROR:
		LOG_INF("AWS_IOT_EVT_ERROR, %d", evt->data.err);
		FATAL_ERROR();
		break;
	default:
		LOG_WRN("Unknown AWS IoT event type: %d", evt->type);
		break;
	}
}

static int app_topics_subscribe(void)
{
	int err;
	static const struct mqtt_topic topic_list[] = {
		{
			.topic.utf8 = MY_CUSTOM_TOPIC_1,
			.topic.size = sizeof(MY_CUSTOM_TOPIC_1) - 1,
			.qos = MQTT_QOS_0_AT_MOST_ONCE,
		}
	};

	err = aws_iot_application_topics_set(topic_list, ARRAY_SIZE(topic_list));
	if (err) {
		LOG_ERR("aws_iot_application_topics_set, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	return 0;
}

// Functions that are executed on specific connection-related events

static void on_aws_iot_evt_connected(const struct aws_iot_evt *const evt)
{
	(void)k_work_cancel_delayable(&connect_work);

	// If persistent session is enabled, the AWS IoT library will not subscribe to any topics.
	// Topics from the last session will be used.
	if (evt->data.persistent_session) {
		LOG_WRN("Persistent session is enabled, using subscriptions "
			"from the previous session");
	}

	// Mark image as working to avoid reverting to the former image after a reboot
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	boot_write_img_confirmed();
#endif
}

static void on_aws_iot_evt_disconnected(void)
{
	(void)k_work_reschedule(&connect_work, K_SECONDS(5));
}

static void connect_work_fn(struct k_work *work)
{
	int err;

	LOG_INF("Connecting to AWS IoT");

	err = aws_iot_connect(NULL);
	if (err == -EAGAIN) {
		LOG_INF("Connection attempt timed out, "
			"Next connection retry in %d seconds",
			CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS);

		(void)k_work_reschedule(&connect_work,
				K_SECONDS(CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS));
	} else if (err) {
		LOG_ERR("aws_iot_connect, error: %d", err);
		FATAL_ERROR();
	}
}

int mqtt_appclient_init(void)
{
	int err;

    _cb = NULL;

	err = aws_iot_init(aws_iot_event_handler);
	if (err) {
		LOG_ERR("AWS IoT library could not be initialized, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	// Add application specific non-shadow topics to the AWS IoT library.
	// These topics will be subscribed to when connecting to the broker.
	err = app_topics_subscribe();
	if (err) {
		LOG_ERR("Adding application specific topics failed, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	return 0;
}

void mqtt_appclient_reg_glucose_cb(glucose_cb_t cb)
{
    if (cb)
    {
        _cb = cb;
    }
}

void mqtt_appclient_connection_start(void)
{
    k_work_reschedule(&connect_work, K_SECONDS(5));
}

void mqtt_appclient_connection_stop(void)
{
	aws_iot_disconnect();
	k_work_cancel_delayable(&connect_work);
}