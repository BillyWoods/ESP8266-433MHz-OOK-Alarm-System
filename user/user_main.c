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

/* definition to expand macro then apply to pragma message */
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

#pragma message(VAR_NAME_VALUE(BIT(2)))
#pragma message(VAR_NAME_VALUE(GPIO_ID_PIN(2)))
#pragma message(VAR_NAME_VALUE(GPIO_PIN_ADDR(2)))
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

    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);

    gpio_init();
    init_ook_decoder();
    gpio_intr_handler_register(ook_intr_handler, (void*) &unprocessedPackets);
}


static bool level = 0;
static void ICACHE_FLASH_ATTR loop(os_event_t* events)
{
    os_printf("%d\r\n", unprocessedPackets.top);
    while(packets_available(&unprocessedPackets))
    {
        uint32* packet = (uint32*) os_malloc(sizeof(uint32)); 
        *packet = packet_pop(&unprocessedPackets);
        
        os_printf("p: %x\r\n", *packet);
        char source[40];
        ets_strcpy(source, ook_ID_to_name(*packet));
        if (source != NULL)
            os_printf("    |>%s\r\n", source);
        
        os_free(packet);
    }

    //at least some delay is crucial so the os has time to do its own thing
    os_delay_us(500000);
    os_delay_us(500000);

    //this function will call itself to create a loop
    system_os_post(user_procTaskPrio, 0, 0);
}
