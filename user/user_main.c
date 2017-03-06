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
#include "webserver.h"
#include "email.h"

// define a macro which will expand defined constants
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

//#pragma message(VAR_NAME_VALUE(LOCAL))

// first and only user process/task
static void loop(os_event_t *events);

packetStack_s unprocessedPackets = { .top = -1 };

/*
    for keeping track (by name) of which sensors have been
    triggered with no duplicate entries, the iter keeps track 
    of the last element
*/
int triggeredSensorsIter = 0;
char* triggeredSensors[NUM_SENSORS];
void clearTriggeredSensors();

/* 
    all the inbuilt string copiers struggle if a null pointer 
    is passed to them so here's a safer version, but the dangerous 
    part is having to manually free the memory it allocates when 
    done with it
*/
char* my_strdup(const char* in)
{
    if (in == NULL)
        return NULL;

    size_t len = strlen(in);
    char* retVal = (char*) os_malloc(sizeof(char)*(len + 1));
    memcpy(retVal, in, sizeof(char)*(len + 1));
    return retVal;
}

void user_rf_pre_init(void)
{
    //nothing
}

void ICACHE_FLASH_ATTR user_init()
{
    // init UART for debugging baud rate comes from user_config.h
    uart_div_modify(0, UART_CLK_FREQ / BAUD_RATE);
    os_printf("\r\nESP8266 OOK decoding\r\n");

    // speed boost (hopefully)
    system_update_cpu_freq(160);

    // setup loop callback in system task queue
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);

    // init wifi using creds in wifi_config.h. Put your own creds in the file
    connect_wifi(WIFI_SSID, WIFI_PSK);

    // webserver-related initialisation
    init_web_server();
    attach_btn_clear(clearTriggeredSensors);

    // email-related stuff
    init_email();

    // init stuff for the ook decoder
    gpio_init();
    gpio_intr_handler_register(ook_intr_handler, (void*) &unprocessedPackets);
    init_ook_decoder();

    os_printf("init finished!\r\n");
}



// needs global declaration for persistence as this pointer
// is handed to webserver.c so shoulddn't be cleared at any point
char generatedWebpage[500];

// called when the "clear" button is pressed on webpage
void clearTriggeredSensors()
{
    int i;
    for(i = 0; i<triggeredSensorsIter; i++)
        os_free(triggeredSensors[i]);
    triggeredSensorsIter = 0;
    set_webpage(NULL);
}

void updateWebpage()
{ 
    int i;
    char* temp = "<title>ESP8266 Home Monitor</title>\n<b>Triggered Sensors:</b><br><ol>\n";

    for (i = 0; i < triggeredSensorsIter; i++)
    {
        os_sprintf(generatedWebpage, "%s<li>%s</li>\n", temp, triggeredSensors[i]);
        temp = generatedWebpage;
    }

    // the clear button
    os_sprintf(generatedWebpage, 
        "%s</ol>\n<form action=\"clear\"><input type=\"submit\" value=\"Clear\"></form>", 
        temp);
    set_webpage(generatedWebpage);
}


static void ICACHE_FLASH_ATTR loop(os_event_t* events)
{
    // check unprocessed OOK packets to see if there are any newly
    // triggered sensors
    while(packets_available(&unprocessedPackets))
    {
        uint32 packet = packet_pop(&unprocessedPackets);
        char* source = my_strdup(ook_ID_to_name(packet));
        bool newTriggering = true;

#ifdef PRINT_OOK_PACKETS_DEBUG
        os_printf("%x\r\n", packet);
#endif

        if (source != NULL)
        {
            int i, j;
            for (i = 0; i < triggeredSensorsIter; i++)
            {
                if (triggeredSensors[i] != NULL)
                    for (j = 0; source[j] == triggeredSensors[i][j]; j++)
                    {
                        // reached the end of both strings and they've been the
                        // same all along
                        if (source[j] == '\0')
                            newTriggering = false;
                    }
            }
            if (newTriggering)
            {
                triggeredSensors[triggeredSensorsIter++] = source;
                // update webpage if there's a new triggering
                updateWebpage();
            }
            else
                os_free(source);
        }
    }
    

    //at least some delay is crucial so the os has time to do its own thing
    os_delay_us(500000);

    //check free memory (for debugging)
    os_printf("free mem: %d\r\n", system_get_free_heap_size());

    //this function will call itself to create a loop
    system_os_post(user_procTaskPrio, 0, 0);
}
