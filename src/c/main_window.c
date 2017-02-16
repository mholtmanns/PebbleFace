#include <pebble.h>
#include "watch_global.h"
#include "main_window.h"

static Window *s_window;
static Layer *s_canvas;
static Layer *s_battery_layer;
static Layer *s_date_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static int s_hours, s_minutes, s_seconds;
static int date_persist;
static BatteryChargeState s_battery_state;
static int s_charging;

static GPath *s_minute_hand_path_ptr = NULL;
static GPath *s_hour_hand_path_ptr = NULL;
static GPath *s_sec_hand_in_path_ptr = NULL;
static GPath *s_sec_hand_out_path_ptr = NULL;

static int32_t get_angle_for_hour(int hour) {
    // Progress through 12 hours, out of 360 degrees
    return (hour * 360) / 12;
}

static int32_t get_angle_for_minute(int minute) {
    // Progress through 60 minutes, out of 360 degrees
    return (minute * 360) / 60;
}

GColor battery_color(int val) {
    switch (val) {
        case 0:
            return GColorRed;
        case 1:
            return GColorOrange;
        case 2:
            return GColorChromeYellow;
        case 3:
            return GColorRajah;
        case 4:
            return GColorYellow;
        case 5:
            return GColorSpringBud;
        case 6:
            return GColorInchworm;
        case 7:
            return GColorBrightGreen;
        case 8:
            return GColorGreen;
        default:
            return GColorIslamicGreen;
    }
}

static void battery_layer_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    GPoint center = grect_center_point(&bounds);
    int x1, y1, x2, y2, radial;
    int charge_state;
    
    if (s_battery_state.is_charging && s_battery_state.charge_percent < 99) {
        // trigger the dirty marking in the canvas updates
        s_charging += 10;
        if (s_charging > 100) {
            s_charging = 10;
        }
    // Not connected to a charger or full, so reset the charging flag
    } else if (s_charging > 0) {
        s_charging = 0;
    }
    
    // Only animate charging if we also show the seconds hand
    charge_state = (s_charging > 0 && s_show_seconds)
        ? s_charging
        : s_battery_state.charge_percent;
    
    /****
     * Calculate the radial from 0 to 120 degrees
     * split into 4 segments between the 4, 5, 6, 7 and 8 hour marks
     * use 8 different colors depending on charge level
     * if we animate charging, do so per second in 15 degree segments
     ****/
    GRect frame = grect_inset(bounds, GEdgeInsets(10 * INSET));
    radial = charge_state * 120 / 100;
    charge_state = charge_state / 10;
    graphics_context_set_fill_color(ctx, battery_color(charge_state));
    graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 5, DEG_TO_TRIGANGLE(120), DEG_TO_TRIGANGLE(120 + radial));
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(120), DEG_TO_TRIGANGLE(120 + radial));
    frame = grect_inset(frame, GEdgeInsets(5));
    graphics_draw_arc(ctx, frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(120), DEG_TO_TRIGANGLE(120 + radial));
#if PBL_ROUND
    int radius = 35;
#else
    int radius = 37;
#endif
    y1 = (-cos_lookup(DEG_TO_TRIGANGLE(120)) * radius / TRIG_MAX_RATIO) + center.y;
    x1 = (sin_lookup(DEG_TO_TRIGANGLE(120)) * radius / TRIG_MAX_RATIO) + center.x;
    y2 = (-cos_lookup(DEG_TO_TRIGANGLE(120)) * (radius + 5) / TRIG_MAX_RATIO) + center.y;
    x2 = (sin_lookup(DEG_TO_TRIGANGLE(120)) * (radius + 5) / TRIG_MAX_RATIO) + center.x;
    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
    y1 = (-cos_lookup(DEG_TO_TRIGANGLE(120 + radial)) * radius / TRIG_MAX_RATIO) + center.y;
    x1 = (sin_lookup(DEG_TO_TRIGANGLE(120 + radial)) * radius / TRIG_MAX_RATIO) + center.x;
    y2 = (-cos_lookup(DEG_TO_TRIGANGLE(120 + radial)) * (radius + 5) / TRIG_MAX_RATIO) + center.y;
    x2 = (sin_lookup(DEG_TO_TRIGANGLE(120 + radial)) * (radius + 5) / TRIG_MAX_RATIO) + center.x;
    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
}

static void date_layer_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    GPoint center = grect_center_point(&bounds);
    GRect dateframe = GRect(center.x - 13, 45, 26, 20);
    char date_str[3];
    
    graphics_context_set_stroke_color(ctx, DATE_COLOR);
    graphics_context_set_text_color(ctx, DATE_COLOR);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_round_rect(ctx, dateframe, 3);
    dateframe = GRect(dateframe.origin.x, dateframe.origin.y - 3, dateframe.size.w, dateframe.size.h);
    (void)snprintf(date_str, sizeof(date_str), date_persist < 10 ? "0%d" : "%d", date_persist);
    graphics_draw_text(ctx, date_str, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       dateframe, GTextOverflowModeWordWrap,
                       GTextAlignmentCenter, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int x, y;
  
  // Minute hand
  int minute_angle = get_angle_for_minute(s_minutes);
  // Correct angle since the base path is rotated by 90 degrees to the 9 o'clock position
  minute_angle = minute_angle < 270 ? minute_angle + 90 : minute_angle - 270; 
  graphics_context_set_fill_color(ctx, MINUTES_COLOR);
  gpath_rotate_to(s_minute_hand_path_ptr, DEG_TO_TRIGANGLE(minute_angle));
  gpath_move_to(s_minute_hand_path_ptr, center);
  gpath_draw_filled(ctx, s_minute_hand_path_ptr);
  gpath_draw_outline(ctx, s_minute_hand_path_ptr);

  // Hour hand
  int hour_angle = get_angle_for_hour(s_hours);
  // Correct angle since the base path is rotated by 90 degrees to the 9 o'clock position
  hour_angle = hour_angle < 270 ? hour_angle + 90 : hour_angle - 270;
  // Move between hours according to original minute angle
  hour_angle = hour_angle + (get_angle_for_minute(s_minutes) / 12);
  graphics_context_set_fill_color(ctx, HOURS_COLOR);
  gpath_rotate_to(s_hour_hand_path_ptr, DEG_TO_TRIGANGLE(hour_angle));
  gpath_move_to(s_hour_hand_path_ptr, center);
  gpath_draw_filled(ctx, s_hour_hand_path_ptr);
  gpath_draw_outline(ctx, s_hour_hand_path_ptr);

  // Seconds hand
  if (s_show_seconds) {
      int seconds_angle = get_angle_for_minute(s_seconds);
      int seconds_length = PBL_IF_ROUND_ELSE(39, 31);
      graphics_context_set_fill_color(ctx, SECONDS_COLOR);
      graphics_context_set_stroke_color(ctx, SECONDS_COLOR);
      y = (-cos_lookup(DEG_TO_TRIGANGLE(seconds_angle)) * seconds_length / TRIG_MAX_RATIO) + center.y;
      x = (sin_lookup(DEG_TO_TRIGANGLE(seconds_angle)) * seconds_length / TRIG_MAX_RATIO) + center.x;
      // Start off with the empty Circle
#if PBL_ROUND
      GRect frame = GRect(x - 11, y - 11, 22, 22);
      graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 5, 0, DEG_TO_TRIGANGLE(360));
#else
      GRect frame = GRect(x - 8, y - 8, 16, 16);
      graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 4, 0, DEG_TO_TRIGANGLE(360));
#endif
      // Now the hands, again adjusted for 90 degrees offset
      seconds_angle = seconds_angle < 270 ? seconds_angle + 90 : seconds_angle - 270;
      gpath_rotate_to(s_sec_hand_in_path_ptr, DEG_TO_TRIGANGLE(seconds_angle));
      gpath_rotate_to(s_sec_hand_out_path_ptr, DEG_TO_TRIGANGLE(seconds_angle));
      gpath_move_to(s_sec_hand_in_path_ptr, center);
      gpath_move_to(s_sec_hand_out_path_ptr, center);
      gpath_draw_filled(ctx, s_sec_hand_in_path_ptr);
      gpath_draw_outline(ctx, s_sec_hand_in_path_ptr);
      gpath_draw_filled(ctx, s_sec_hand_out_path_ptr);
      gpath_draw_outline(ctx, s_sec_hand_out_path_ptr);
  }
    // Take the seconds tick counter to update the battery charge indicator
  if (s_charging > 0) {
      layer_mark_dirty(s_battery_layer);
  }
}

static void handle_battery(BatteryChargeState charge_state) {
  s_battery_state = charge_state;
  layer_mark_dirty(s_battery_layer);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Create GBitmap
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);

    // Create BitmapLayer to display the GBitmap
    s_background_layer = bitmap_layer_create(bounds);

    // Set the bitmap onto the layer and add to the window
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

    // subscribe to battery status
    battery_state_service_subscribe(handle_battery);
    // Create the battery gauge layer
    s_battery_layer = layer_create(bounds);
    layer_set_update_proc(s_battery_layer, battery_layer_update_proc);
    layer_add_child(window_layer, s_battery_layer);
    // trigger an update now
    s_battery_state = battery_state_service_peek();
    layer_mark_dirty(s_battery_layer);

    // init the date layer
    date_persist = -1;
    s_date_layer = layer_create(bounds);
    layer_set_update_proc(s_date_layer, date_layer_update_proc);
    layer_add_child(window_layer, s_date_layer);

    // Create the hands paths
    s_minute_hand_path_ptr = gpath_create(&MINUTE_HAND_PATH);
    s_hour_hand_path_ptr = gpath_create(&HOUR_HAND_PATH);
    s_sec_hand_in_path_ptr = gpath_create(&SECOND_HAND_IN_PATH);
    s_sec_hand_out_path_ptr = gpath_create(&SECOND_HAND_OUT_PATH);

    // Create canvas for the Hands
    s_canvas = layer_create(bounds);
    layer_set_update_proc(s_canvas, canvas_update_proc);
    layer_add_child(window_layer, s_canvas);
}

static void window_unload(Window *window) {
    // cleanup
    gpath_destroy(s_hour_hand_path_ptr);
    gpath_destroy(s_minute_hand_path_ptr);
    gpath_destroy(s_sec_hand_in_path_ptr);
    gpath_destroy(s_sec_hand_out_path_ptr);
    layer_destroy(s_canvas);
    bitmap_layer_destroy(s_background_layer);
    gbitmap_destroy(s_background_bitmap);
    battery_state_service_unsubscribe();
    window_destroy(s_window);
}

void main_window_push() {
  s_window = window_create();
  window_set_background_color(s_window, BG_COLOR);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

void main_window_update(int date, int hours, int minutes, int seconds) {
    s_hours = hours;
    s_minutes = minutes;
    s_seconds = seconds > 59 ? 0 : seconds;

    layer_mark_dirty(s_canvas);
    // In case we got a day change, update the date field
    if (date != date_persist) {
        date_persist = date;
        layer_mark_dirty(s_date_layer);
    }
}

void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
    // Read boolean preferences
    Tuple *second_tick_t = dict_find(iter, MESSAGE_KEY_SecondTick);
    if(second_tick_t) {
        int enable_seconds = second_tick_t->value->int32 == 1;
        if (enable_seconds != s_show_seconds) {
            s_show_seconds = enable_seconds;
            tick_timer_service_subscribe(enable_seconds ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
        }
    }

    Tuple *animations_t = dict_find(iter, MESSAGE_KEY_Animations);
    if(animations_t) {
        bool animations = animations_t->value->int32 == 1;
    }
}