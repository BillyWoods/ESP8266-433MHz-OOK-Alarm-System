#ifndef OOK_DECODE_H
#define OOK_DECODE_H

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"

typedef volatile struct packetStack
{
    uint32 packets[256];
    int top; 
} packetStack_s;

void packet_push(uint32 packet, packetStack_s* ps);
uint32 packet_pop(packetStack_s* ps);
bool packets_available(packetStack_s* ps);

//setup interrupts and gpio 2 as input
void init_ook_decoder();
void ook_intr_handler(uint32 intrMask, void* packets);

#endif
