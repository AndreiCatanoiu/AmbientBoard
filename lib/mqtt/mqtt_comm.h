#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "settings_store.h"

#define MQTT_TOPIC_PREFIX  "ambientboard"

void mqtt_comm_init(void);
void mqtt_comm_reconnect(void);

bool mqtt_is_connected(void);
bool mqtt_has_device_name(void);

void mqtt_publish_message(const char *text);
