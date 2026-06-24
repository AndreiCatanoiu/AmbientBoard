#pragma once

#include <stdint.h>
#include <stdbool.h>

void display_init(void);

void display_task(void *pvParameters);

bool display_lock(uint32_t timeout_ms);
void display_unlock(void);

void display_wake(void);
bool display_is_awake(void);
