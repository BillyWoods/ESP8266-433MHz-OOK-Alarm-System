#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

#define WEBPAGE_SIZE 1024

typedef void (*button_pressed_cb)(void);

void ICACHE_FLASH_ATTR connect_wifi(const char* ssid, const char* password);
bool                   is_wifi_connected();
void ICACHE_FLASH_ATTR init_web_server();
// this function will not automatically cleanup the string passed to it once used
void ICACHE_FLASH_ATTR set_webpage(char* html);
void ICACHE_FLASH_ATTR attach_btn_clear(button_pressed_cb onClearFunc);
void ICACHE_FLASH_ATTR attach_btn_arm_alarm(button_pressed_cb armAlarmFunc);
void ICACHE_FLASH_ATTR attach_btn_disarm_alarm(button_pressed_cb disarmAlarmFunc);

// don't call these handlers directly, they are just here for predef
void ICACHE_FLASH_ATTR server_handle_new_conn(void* arg);
void ICACHE_FLASH_ATTR server_handle_recv_data(void* arg, char* recvData, unsigned short len);

#endif
