#include <pebble.h>
#include "watch_global.h"
#include "main_window.h"

static void tick_handler(struct tm *time_now, TimeUnits changed) {
  main_window_update(time_now->tm_mday, time_now->tm_hour, time_now->tm_min, time_now->tm_sec);
}

static void init() {
  main_window_push();
  if (s_show_seconds)
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  else
      tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  tick_timer_service_unsubscribe();
}

int main() {
  init();
  app_event_loop();
  deinit();
}