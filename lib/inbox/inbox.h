#pragma once

#include <stdint.h>
#include <stddef.h>

#include "settings_store.h"

#define INBOX_LINE_LEN   64
#define INBOX_FROM_LEN   MQTT_DEVICE_NAME_LEN
#define INBOX_ITEM_MAX   10

void     inbox_init(void);
void     inbox_refresh(void);

uint8_t  inbox_count(void);
void     inbox_get(uint8_t index, char *from, char *line, size_t len);
uint32_t inbox_version(void);
