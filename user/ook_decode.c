#include "ook_decode.h"

void packet_push(uint32 packet, packetStack_s* ps)
{
    ps->top++;
    ps->packets[ps->top] = packet;
}

uint32 packet_pop(packetStack_s* ps)
{
    if (packets_available(ps))
    {
        ps->top--;
        return ps->packets[ps->top + 1];
    }
    else
        return -1;
}

bool packets_available(packetStack_s* ps)
{
    return ps->top >= 0;
}

void init_ook_decoder()
{
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);
    //set gpio 2 as input
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(2));
    //interrupt on rising edges
    gpio_pin_intr_state_set(GPIO_ID_PIN(2), GPIO_PIN_INTR_POSEDGE);
}


/*
variables related to the operation of decoding on interrupt
*/
//used to calculate the width of a high pulse
static uint32 lastPosEdgeTime = 0;
//used to see how long there has been no signal for (useful for detecting the end of a packet)
static uint32 lastNegEdgeTime = 0;
//Are we interrupting on posedges or negedges at present?
static bool thisIntrRise = true;

static uint32 packetBuffer = 0; //all packets with this alarm system are 24 bits long so fit nicely 
static char packetBuffHead = 0;

void ook_intr_handler(uint32 intrMask, void* packets)
{
    uint32 begintime = system_get_time();

    packetStack_s* ps = packets;
    //was it gpio 2 that triggered the interupt?
    if (intrMask & BIT(2))
    {
        if (thisIntrRise)
        {
            lastPosEdgeTime = system_get_time();

            uint32 lowDuration = lastPosEdgeTime - lastNegEdgeTime;
            //os_printf("low dur: %d\r\n", lowDuration);
            //End of burst/packet
            if (lowDuration > 7000 && lowDuration < 20000)
            {
                //clear packet buffer and send packet through if packet length is long enough to not simply be noise
                if (packetBuffHead > 10)
                {
                    packet_push(packetBuffer, ps);
                }
                packetBuffer = 0;
                packetBuffHead = 0;
            }
            //end of transmission
            else if (lowDuration >= 20000)
            {
                //clear packet buffer and send packet through if packet length is long enough to not simply be noise
                if (packetBuffHead > 10)
                {
                    packet_push(packetBuffer, ps);
                }

                //os_printf("\r\nend of transmission\r\n");
                packetBuffer = 0;
                packetBuffHead = 0;
            }
        }
        else
        {
            lastNegEdgeTime = system_get_time();

            uint32 pulseDuration = lastNegEdgeTime - lastPosEdgeTime;
            
            // Being quite specific about pulse timings here, so this will not necessarily decode other 
            // transmitters without adjustements
            
            //A long pulse
            if (pulseDuration > 690 && pulseDuration < 1100)
            {
                //write a 1
                packetBuffer |= (1 << packetBuffHead++);
            }
            //A short pulse
            else if (pulseDuration <= 400 && pulseDuration >= 180)
            {
                packetBuffHead++;
            }
            // discard packet because it is indecipherable or noise
            else
            {
                packetBuffer = 0;
                packetBuffHead = 0;
            }
        }
        // Reactivate interrupts for GPIO0
        int edgeTrig = thisIntrRise ? GPIO_PIN_INTR_NEGEDGE : GPIO_PIN_INTR_POSEDGE; 
        //update record of next interrupt's type
        thisIntrRise = !thisIntrRise;
        gpio_intr_ack(BIT(2));
        gpio_pin_intr_state_set(GPIO_ID_PIN(2), edgeTrig);
    }
}
