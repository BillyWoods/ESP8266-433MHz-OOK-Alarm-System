#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"

#include "user_config.h"
#include "wifi_config.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
//first and only user process/task
static void loop(os_event_t *events);


void user_rf_pre_init(void)
{
    //nothing
}

void ICACHE_FLASH_ATTR user_init()
{
    //init UART for debugging baud rate comes from user_config.h
    uart_div_modify(0, UART_CLK_FREQ / BAUD_RATE);
    os_printf("ESP8266 interrupt test");

    gpio_init();

    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);
}

static void ICACHE_FLASH_ATTR loop(os_event_t* events)
{
    os_printf("hello there!");

    //at least some delay is crucial so the os has time to do its own thing
    os_delay_us(100000);

    //this function will call itself to create a loop
    system_os_post(user_procTaskPrio, 0, 0);
}
