#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "user_interface.h"

#define BAUD_RATE 115200 
#define TIMEZONE 11
// 60 seconds from trigerring to email being sent ( allows
//   time for alarm to be disarmed)
#define ALERT_EMAIL_DELAY 60

#define user_procTaskPrio       0
#define ssl_disconnectPrio      1
#define user_procTaskQueueLen   2 
os_event_t user_procTaskQueue[user_procTaskQueueLen];

/* 
    uncomment the line below to get the ESP8266 to print all
    packets it receives to the UART. This is how you can find
    the IDs of your sensors 
*/
//#define PRINT_OOK_PACKETS_DEBUG
#define PRINT_EMAIL_DEBUG
#endif
