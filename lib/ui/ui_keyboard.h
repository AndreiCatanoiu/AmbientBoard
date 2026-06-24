#pragma once

typedef void (*ui_keyboard_done_cb)(const char *text, void *user);

void ui_keyboard_show(const char *title, const char *initial,
                      ui_keyboard_done_cb cb, void *user);
