#pragma once
#include <pebble.h>

void main_window_push();

void main_window_update(int date, int hours, int minutes, int seconds);
void prv_inbox_received_handler(DictionaryIterator *iter, void *context);
