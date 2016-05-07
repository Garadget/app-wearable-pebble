#include <pebble.h>

#include "appmessage.h"
#include "devicelist.h"

static void init(void) {
	appmessage_init();
	device_window_init();
}

static void deinit(void) {
 	device_window_destroy();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
