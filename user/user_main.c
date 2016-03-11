#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"

#include "user_config.h"
#include "wifi_config.h"
#include "ook_sensor_IDs.h"
#include "ook_decode.h"
//#include "webserver.h"

/* definition to expand macro then apply to pragma message */
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

#pragma message(VAR_NAME_VALUE(STATION_WRONG_PASSWORD))
#pragma message(VAR_NAME_VALUE(STATION_NO_AP_FOUND))
#pragma message(VAR_NAME_VALUE(STATION_CONNECT_FAIL))
#pragma message(VAR_NAME_VALUE(LOCAL))

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
//first and only user process/task
static void loop(os_event_t *events);

packetStack_s unprocessedPackets = { .top = -1 };

void user_rf_pre_init(void)
{
    //nothing
}

void ICACHE_FLASH_ATTR user_init()
{
    //init UART for debugging baud rate comes from user_config.h
    uart_div_modify(0, UART_CLK_FREQ / BAUD_RATE);
    os_printf("\r\nESP8266 OOK decoding\r\n");

    //setup loop callback in system task queue
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);

    //init wifi using creds in wifi_config.h. Put your own creds in the file
    wifi_station_set_auto_connect(0);
    //connect_wifi(WIFI_SSID, WIFI_PSK);

    //init stuff for the ook decoder
    gpio_init();
    gpio_intr_handler_register(ook_intr_handler, (void*) &unprocessedPackets);
    init_ook_decoder();

    os_printf("init finished!");
}


//static bool level = 0;

static void ICACHE_FLASH_ATTR loop(os_event_t* events)
{
    //os_printf("wifi up: %s\r\n", is_wifi_connected()? "yes" : "no");
    //os_printf("%d\r\n", wifi_station_get_connect_status());
    
    while(packets_available(&unprocessedPackets))
    {
        uint32* packet = (uint32*) os_malloc(sizeof(uint32)); 
        *packet = packet_pop(&unprocessedPackets);
        
        char source[40];
        ets_strcpy(source, ook_ID_to_name(*packet));
        if (source != NULL)
            os_printf("    |>%s\r\n", source);
        
        os_free(packet);
    }

    //at least some delay is crucial so the os has time to do its own thing
    os_delay_us(500000);

    //this function will call itself to create a loop
    system_os_post(user_procTaskPrio, 0, 0);
}
