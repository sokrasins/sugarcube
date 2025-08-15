#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include "status.h"

typedef void (*glucose_cb_t)(int glucose);

int mqtt_appclient_init(void);

void mqtt_appclient_reg_glucose_cb(glucose_cb_t cb);

void mqtt_appclient_connection_start(void);

void mqtt_appclient_connection_stop(void);

#endif /*MQTT_CLIENT_H_*/