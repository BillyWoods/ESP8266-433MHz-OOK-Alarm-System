#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"

#include "user_config.h"
#include "wifi_config.h"
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
    gpio_intr_handler_register(ook_intr_handler, &unprocessedPackets);

}


static bool level = 0;
static void ICACHE_FLASH_ATTR loop(os_event_t* events)
{
    //os_printf("interrupts: %d\r\n", interruptArg);
    //os_printf("RTC: %d\r\n", system_get_time());
    
    //level = !(level);
    //GPIO_OUTPUT_SET(GPIO_ID_PIN(2), level);
    //at least some delay is crucial so the os has time to do its own thing
    os_delay_us(500000);

    //this function will call itself to create a loop
    system_os_post(user_procTaskPrio, 0, 0);
}
