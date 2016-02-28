#ifndef OOK_SENSOR_IDS_H
#define OOK_SENSOR_IDS_H

//change this according to the number of entries below
static const int numSensors = 8;

//enter your own system's codes
static const uint32 recognisedIDs[numSensors] = {
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000,
    0x000000
};

static const char* recognisedIDsLookup[numSensors] = {
    "garage door",
    "hallway PIR",
    "laundry door",
    "front door",
    "kitchen PIR",
    "lounge PIR",
    "disarm alarm",
    "arm alarm"
};

char* ook_ID_to_name(uint32 code)
{
    for (int i = 0; i<numSensors; i++)
    {
        if(code == recognisedIDs[i])
            return recognisedIDsLookup[i];
    }
}

#endif
