#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define WIFI_SCAN_MAX 16

typedef struct {
    char    ssid[33];
    uint8_t strength;   
    uint8_t open;       
} wifi_scan_entry_t;

esp_err_t wifi_init(void);
void wifi_task(void *pvParameters);

bool wifi_is_connected(void);
uint8_t wifi_scan(wifi_scan_entry_t *entries, uint8_t max);
void wifi_connect_to(const char *ssid, const char *pass);
void wifi_disconnect(void);
void wifi_reconnect(void);
bool wifi_has_saved(void);

void    wifi_get_ip_string(char *out, uint8_t len);
uint8_t wifi_get_strength(void);
void    wifi_get_ssid(char *out, uint8_t len);