#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MQTT_TOPIC_IN   "ambientboard/in"
#define MQTT_TOPIC_OUT  "ambientboard/out"

#define MQTT_MSG_MAX 20
#define MQTT_MSG_LEN 80

void mqtt_comm_init(void);

bool mqtt_is_connected(void);

void mqtt_publish_message(const char *text);

uint8_t mqtt_get_message_count(void);
void    mqtt_get_message(uint8_t index, char *out, uint8_t len);

uint32_t mqtt_get_version(void);
