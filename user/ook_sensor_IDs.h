#ifndef OOK_SENSOR_IDS_H
#define OOK_SENSOR_IDS_H

//change this according to the number of entries below
#define NUM_SENSORS 8
#define ARM_CODE 0x000000
#define DISARM_CODE 0x000000

//enter your own system's codes
const uint32 recognisedIDs[NUM_SENSORS] = {
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    DISARM_CODE,
    ARM_CODE 
};

const char* recognisedIDsLookup[NUM_SENSORS] = {
    "garage door",
    "hallway PIR",
    "laundry door",
    "front door",
    "kitchen PIR",
    "lounge PIR",
    "disarm alarm",
    "arm alarm"
};

const char* ook_ID_to_name(uint32 code)
{
    int i;
    for (i = 0; i<NUM_SENSORS; i++)
    {
        if(code == recognisedIDs[i])
        {
            return (recognisedIDsLookup[i]);
        }
    }
    return NULL;
}

#endif
