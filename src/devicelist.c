#include <pebble.h>
#include "devicelist.h"
#include "pebble-assist.h"
#include "common.h"
#define MAX_DEVICES 10

static int device_num = 0;
static int num_devices;
static char devices[MAX_DEVICES][20];
static char* action;
static GBitmap *down_icon, *up_icon, *query_icon, *logo, *garage_open, *garage_closed, *garage_halfopen, *garage_query;
ActionBarLayer *action_bar_layer;
static void request_action();
static void window_load(Window *window);
static void window_unload(Window *window);
static void device_window_load(Window *window);
static void device_window_unload(Window *window);
void action_bar_click_config_provider(void *context);
void window_click_config_provider(void *context);

static Window *device_window;
static BitmapLayer *garage_image_layer;
static TextLayer *device_layer, *status_layer, *signal_strength_layer, *time_layer;
static char device_text[32], status_text[32], signal_strength_text[8], time_text[8];

void device_window_init() {
	device_window = window_create();
  window_set_window_handlers(device_window, (WindowHandlers) {
		.load = device_window_load,
    .unload = device_window_unload,
	});
  window_stack_push(device_window, true);
}

void device_window_destroy() {
  window_stack_pop_all(true);
  window_destroy(device_window);
}

void devicelist_in_received_handler(DictionaryIterator *iter) {
  Tuple *command_tuple = dict_find(iter, COMMAND_KEY);
  if (strcmp(command_tuple->value->cstring, "no config") == 0) {
    text_layer_set_text(device_layer, "Please");
    text_layer_set_text(status_layer, "configure.");
  } else if (strcmp(command_tuple->value->cstring, "status") == 0) /* this is a status update */ {
    Tuple *status_tuple = dict_find(iter, DEVICES_KEY);
    if (strcmp(status_tuple->value->cstring, "open") == 0)
      bitmap_layer_set_bitmap(garage_image_layer, garage_open);
    else if (strcmp(status_tuple->value->cstring, "closed") == 0)
      bitmap_layer_set_bitmap(garage_image_layer, garage_closed);
    else
      bitmap_layer_set_bitmap(garage_image_layer, garage_halfopen);
    Tuple *time_tuple = dict_find(iter, DEVICES_KEY+1);
    Tuple *signal_tuple = dict_find(iter, DEVICES_KEY+3);
    snprintf(device_text, sizeof(device_text), "%s", devices[device_num]);
    snprintf(status_text, sizeof(status_text), "%s", status_tuple->value->cstring);
    snprintf(time_text, sizeof(time_text), "%s", time_tuple->value->cstring);
    snprintf(signal_strength_text, sizeof(signal_strength_text), "%sdB", signal_tuple->value->cstring);
    text_layer_set_text(signal_strength_layer, signal_strength_text);
    text_layer_set_text(device_layer, device_text);
    text_layer_set_text(status_layer, status_text);
    text_layer_set_text(time_layer, time_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"%s door is %s.", devices[device_num], status_tuple->value->cstring);
   } else /* this is a list of devices */ {
    num_devices = 0;
    for (int deviceNumber=0; deviceNumber < MAX_DEVICES; deviceNumber++) {
      Tuple *device_tuple = dict_find(iter, DEVICES_KEY+deviceNumber);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Finding device %d.", deviceNumber);
      if ((device_tuple) && (strlen(device_tuple->value->cstring) > 0)) {
        strncpy(devices[deviceNumber], device_tuple->value->cstring, sizeof devices[0]);
        num_devices = deviceNumber+1;
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Got location %d = %s.", deviceNumber, devices[deviceNumber]);
      }
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Got %d devices.", num_devices);
    if (num_devices == 0) {
      text_layer_set_text(device_layer, "No doors");
      text_layer_set_text(status_layer, "found.");
    } else {
      text_layer_set_text(device_layer, devices[device_num]);
      action = "query";
      device_num = 0;
      request_action();
    }
  }
}

static void my_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "In Up key handler.");
  action = "open";
  request_action();
}

static void my_select_click_handler(ClickRecognizerRef recognizer, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "In Select key handler.");
  action = "query";
  request_action();
}

static void my_down_click_handler(ClickRecognizerRef recognizer, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "In Down key handler.");
  action = "close";
  request_action();
}

static void my_long_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In Long Up key handler.");
  device_num = (device_num + 1) % num_devices;
  text_layer_set_text(device_layer, devices[device_num]);
  vibes_short_pulse();
  action = "query";
  request_action();
}

// static void my_long_select_click_handler(ClickRecognizerRef recognizer, void *context) {
// 	APP_LOG(APP_LOG_LEVEL_DEBUG, "In Long Select key handler.");
//   action = "stop";
//   request_action();
// }

static void my_long_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In Long Down key handler.");
  device_num = (device_num + num_devices- 1) % num_devices;
  text_layer_set_text(device_layer, devices[device_num]);
  vibes_short_pulse();
  action = "query";
  request_action();
}

void action_bar_click_config_provider(void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In action bar click config provider.");
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) my_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) my_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) my_up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 700, (ClickHandler) my_long_up_click_handler, NULL);
//   window_long_click_subscribe(BUTTON_ID_SELECT, 700, (ClickHandler) my_long_select_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 700, (ClickHandler) my_long_down_click_handler, NULL);
}

static void request_action() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Requesting action %d %s:", device_num, action);
//   bitmap_layer_set_bitmap(garage_image_layer, garage_query);
  text_layer_set_text(status_layer, "...");
  Tuplet request_tuple = TupletCString(COMMAND_KEY, action);
	Tuplet device_tuple = TupletInteger(DEVICE_KEY, device_num);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	if (iter == NULL) return;
	dict_write_tuplet(iter, &request_tuple);
	dict_write_tuplet(iter, &device_tuple);
	dict_write_end(iter);
	app_message_outbox_send();
}

static void device_window_load(Window *window) {
#ifndef PBL_SDK_3
  window_set_fullscreen(window,true);
#endif
  
  Layer *device_window_layer = window_get_root_layer(device_window);
  GRect bounds = layer_get_bounds(device_window_layer);

  action_bar_layer = action_bar_layer_create();
  down_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SMALL_GARAGE_CLOSED);
  query_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_QUERY);
  up_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SMALL_GARAGE_OPEN);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, down_icon);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, query_icon);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, up_icon);
  action_bar_layer_add_to_window(action_bar_layer, device_window);
  action_bar_layer_set_click_config_provider(action_bar_layer, action_bar_click_config_provider);

  garage_image_layer = bitmap_layer_create(GRect(0, 0, bounds.size.w-20, bounds.size.h-44));
  garage_query = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LARGE_GARAGE_QUERY);
  garage_open = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LARGE_GARAGE_OPEN);
  garage_closed = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LARGE_GARAGE_CLOSED);
  garage_halfopen = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LARGE_GARAGE_HALFOPEN);
  layer_add_child(device_window_layer, bitmap_layer_get_layer(garage_image_layer));
  bitmap_layer_set_bitmap(garage_image_layer, garage_query);

  device_layer = text_layer_create(GRect(0, bounds.size.h-64, 124, 32));
  text_layer_set_background_color(device_layer, GColorClear);
  text_layer_set_font(device_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(device_layer, GTextAlignmentCenter);
  text_layer_set_text(device_layer, "Initializing");
  layer_add_child(device_window_layer, text_layer_get_layer(device_layer));

  status_layer = text_layer_create(GRect(0, bounds.size.h-36, 124, 32));
  text_layer_set_background_color(status_layer, GColorClear);
  text_layer_set_font(status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(status_layer, GTextAlignmentCenter);
  text_layer_set_text(status_layer, "...");
  layer_add_child(device_window_layer, text_layer_get_layer(status_layer));

  time_layer = text_layer_create(GRect(4, 0, 54, 20));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(time_layer, GTextAlignmentLeft);
  layer_add_child(device_window_layer, text_layer_get_layer(time_layer));

  signal_strength_layer = text_layer_create(GRect(70, 0, 50, 20));
  text_layer_set_background_color(signal_strength_layer, GColorClear);
  text_layer_set_font(signal_strength_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(signal_strength_layer, GTextAlignmentRight);
  layer_add_child(device_window_layer, text_layer_get_layer(signal_strength_layer));
}

static void device_window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Destroying device window.");

  Tuplet request_tuple = TupletCString(COMMAND_KEY, "clear");
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	if (iter == NULL) return;
	dict_write_tuplet(iter, &request_tuple);
	dict_write_end(iter);
	app_message_outbox_send();

  action_bar_layer_remove_from_window(action_bar_layer);
  action_bar_layer_destroy(action_bar_layer);
  bitmap_layer_destroy(garage_image_layer);
  gbitmap_destroy(down_icon);
  gbitmap_destroy(query_icon);
  gbitmap_destroy(up_icon);
  gbitmap_destroy(garage_open);
  gbitmap_destroy(garage_closed);
  gbitmap_destroy(garage_halfopen);
  gbitmap_destroy(garage_query);
  text_layer_destroy(signal_strength_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(status_layer);
  text_layer_destroy(device_layer);
}
