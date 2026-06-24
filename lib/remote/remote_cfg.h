#pragma once

#include <stddef.h>
#include <stdint.h>

size_t remote_get_host(char *out, size_t len);
uint16_t remote_get_mqtt_port(void);
size_t remote_get_mqtt_user(char *out, size_t len);
size_t remote_get_mqtt_pass(char *out, size_t len);
size_t remote_build_mqtt_uri(char *out, size_t len);
size_t remote_build_ota_url(char *out, size_t len);
