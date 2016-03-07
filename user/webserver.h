#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"

//#include "user_config.h"

void ICACHE_FLASH_ATTR connect_wifi(char* ssid, char* password);
bool                   is_wifi_connected();
void ICACHE_FLASH_ATTR init_web_server();

#endif
