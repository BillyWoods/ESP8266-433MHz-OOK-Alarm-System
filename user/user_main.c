#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"

#include "user_config.h"
#include "wifi_config.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
//first and only user process/task
static void loop(os_event_t *events);

static volatile int interruptArg = 0;
LOCAL void interrupt_handler(int* arg);

void user_rf_pre_init(void)
{
    //nothing
}

void ICACHE_FLASH_ATTR user_init()
{
    //init UART for debugging baud rate comes from user_config.h
    uart_div_modify(0, UART_CLK_FREQ / BAUD_RATE);
    os_printf("\r\nESP8266 OOK decoding\r\n");

    gpio_init();

    //set pin 0 and 2 as gpio
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

    //let gpio 0 float
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO0_U);
    //pull gpio 2 up
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);

    //setting works like so:
    // REG:           01011010
    // MASK:          00000100
    // REG | MASK:    01011110
    //clearing works like so:
    // REG:           01011010
    // MASK:          01000000
    // REG & !(MASK): 00011010
    //
    //set_mask, clear_mask, enable as output mask, clear as output mask (make an input)
    gpio_output_set(0, GPIO_ID_PIN(2), GPIO_ID_PIN(2), GPIO_ID_PIN(0));
    
    ETS_GPIO_INTR_DISABLE();
    //attach our interrupt handler
    ETS_GPIO_INTR_ATTACH(interrupt_handler, &interruptArg);
    //change some bits in gpio0's config register
    /*gpio_register_set(GPIO_PIN_ADDR(0),
                      GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)  |
                      GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE) |
                      GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));*/
    gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_POSEDGE);
    ETS_GPIO_INTR_ENABLE();

    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);
}

//used to calculate the width of a high pulse
static uint32 lastPosEdgeTime = 0;
//used to see how long there has been no signal for (useful for detecting the end of a packet)
static uint32 lastNegEdgeTime = 0;
//Are we interrupting on posedges or negedges?
static bool thisIntrRise = true;

uint32 packetBuffer = 0; //all packets with this alarm system are 24 bits long so fit nicely 
char packetBuffHead = 0;

LOCAL void interrupt_handler(int* arg)
{
    //read the state of all gpio pins (GPIO_STATUS_ADDRESS = PIN_IN = 0x60000318 ... I think)
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    // if the interrupt was by GPIO0
    if (gpio_status & BIT(0))
    {
        // disable interrupt for GPIO0
        gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_DISABLE);
        // Do something, for example, increment arg indirectly
        //(*arg)++;
        if (thisIntrRise)
        {
            lastPosEdgeTime = system_get_time();

            uint32 lowDuration = system_get_time() - lastNegEdgeTime;
            //End of burst/packet
            if (lowDuration > 5000 && lowDuration < 20000)
            {
                //clear packet buffer and send packet through if packet length is 24 bits
                if (packetBuffHead > 10)
                    os_printf("%x\r\n", packetBuffer);
                packetBuffer = 0;
                packetBuffHead = 0;
            }
            //end of transmission
            else if (lowDuration >= 20000)
            {
                //clear packet buffer and send packet through if packet length is 24 bits
                if (packetBuffHead > 5)
                {
                    os_printf("%x\r\n", packetBuffer);
                    os_printf("\r\nend of transmission\r\n");
                }
                packetBuffer = 0;
                packetBuffHead = 0;
            }
        }
        else
        {
            lastNegEdgeTime = system_get_time();

            uint32 pulseDuration = system_get_time() - lastPosEdgeTime;
            
            //Being quite selective about pulse timings here
            //A long pulse
            if (pulseDuration > 690 && pulseDuration < 1100)
            {
                //write a 1
                packetBuffer |= (1 << packetBuffHead++);
            }
            //A short pulse
            else if (pulseDuration <= 320 && pulseDuration >= 180)
            {
                packetBuffHead++;
            }
            //discard packet (clear packet buffer)
            else
            {
                packetBuffer = 0;
                packetBuffHead = 0;
            }
        }
        //clear interrupt status for GPIO0
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(0));
        // Reactivate interrupts for GPIO0
        int edgeTrig = thisIntrRise ? GPIO_PIN_INTR_NEGEDGE : GPIO_PIN_INTR_POSEDGE; 
        //update record of next interrupt's type
        thisIntrRise = !thisIntrRise;
        gpio_pin_intr_state_set(GPIO_ID_PIN(0), edgeTrig);
    }

}
 

static bool level = 0;
static void ICACHE_FLASH_ATTR loop(os_event_t* events)
{
    //os_printf("interrupts: %d\r\n", interruptArg);
    //os_printf("RTC: %d\r\n", system_get_time());
    
    level = !(level);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(2), level);

    //at least some delay is crucial so the os has time to do its own thing
    os_delay_us(500000);
    //os_printf("\r\n");

    //this function will call itself to create a loop
    system_os_post(user_procTaskPrio, 0, 0);
}
