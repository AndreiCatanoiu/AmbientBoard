#pragma once

#include <stdbool.h>

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CHECKING,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_DONE,
    OTA_STATE_FAILED,
} ota_state_t;

void ota_update_init(void);

bool ota_check_version(void);

bool ota_update_start(void);

ota_state_t ota_get_state(void);
const char *ota_get_status_text(void);

const char *ota_get_latest_version(void);

bool ota_is_update_available(void);
