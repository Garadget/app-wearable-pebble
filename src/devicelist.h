#include "common.h"

#pragma once

void devicelist_init();
void devicelist_destroy(void);
void devicelist_in_received_handler(DictionaryIterator *iter);
