#include <pebble.h>

#include "appmessage.h"
#include "devicelist.h"

static void init(void) {
	appmessage_init();
	devicelist_init();
}

static void deinit(void) {
	devicelist_destroy();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
