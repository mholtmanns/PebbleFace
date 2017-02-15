#pragma once

#include <pebble.h>

#define MINUTES_COLOR   GColorBlack
#define HOURS_COLOR     GColorBlack
#ifdef PBL_COLOR
#define SECONDS_COLOR   GColorRed
#define DATE_COLOR      GColorDarkCandyAppleRed
#else
#define SECONDS_COLOR   GColorBlack
#define DATE_COLOR      GColorBlack
#endif

#define INSET PBL_IF_ROUND_ELSE(5, 3)

static const int s_show_seconds = 0;

#ifdef PBL_ROUND
#define BG_COLOR        GColorWhite

static const GPathInfo HOUR_HAND_PATH = {
  .num_points = 5,
  .points = (GPoint []) {{-50, 0}, {-46, -5}, {10, -5}, {10, 5}, {-46, 5}}
};

static const GPathInfo MINUTE_HAND_PATH = {
  .num_points = 5,
  .points = (GPoint []) {{-75, 0}, {-71, -4}, {10, -3}, {10, 3}, {-71, 4}}
};

static const GPathInfo SECOND_HAND_OUT_PATH = {
  .num_points = 3,
    .points = (GPoint []) {{-78, 0}, {-45, -2}, {-45, 2}}
};

static const GPathInfo SECOND_HAND_IN_PATH = {
  .num_points = 4,
  .points = (GPoint []) {{-31, -2}, {13, -3}, {13, 3}, {-31, 2}}
};

#else
#define BG_COLOR        GColorBlack

static const GPathInfo HOUR_HAND_PATH = {
  .num_points = 5,
  .points = (GPoint []) {{-40, 0}, {-37, -4}, {8, -4}, {8, 4}, {-37, 4}}
};

static const GPathInfo MINUTE_HAND_PATH = {
  .num_points = 5,
  .points = (GPoint []) {{-60, 0}, {-57, -3}, {8, -3}, {8, 3}, {-57, 3}}
};

static const GPathInfo SECOND_HAND_OUT_PATH = {
  .num_points = 3,
  .points = (GPoint []) {{-62, 0}, {-38, -1}, {-38, 1}}
};

static const GPathInfo SECOND_HAND_IN_PATH = {
  .num_points = 4,
  .points = (GPoint []) {{-24, -1}, {10, -2}, {10, 2}, {-24, 1}}
};
#endif