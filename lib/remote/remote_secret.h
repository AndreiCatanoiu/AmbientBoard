#pragma once

#include <stddef.h>
#include <stdint.h>

size_t   rsec_host(char *out, size_t len);
uint16_t rsec_mqtt_port(void);
size_t   rsec_mqtt_user(char *out, size_t len);
size_t   rsec_mqtt_pass(char *out, size_t len);
size_t   rsec_ota_base(char *out, size_t len);
size_t   rsec_pull_url(char *out, size_t len);
