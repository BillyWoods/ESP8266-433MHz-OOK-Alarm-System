#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdlib.h>

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

void ICACHE_FLASH_ATTR connect_wifi(const char* ssid, const char* password);
bool                   is_wifi_connected();
void ICACHE_FLASH_ATTR init_web_server();
// don't call these handlers directly
void ICACHE_FLASH_ATTR server_handle_new_conn(void* arg);
void ICACHE_FLASH_ATTR server_handle_recv_data(void* arg, char* recvData, unsigned short len);

#endif
