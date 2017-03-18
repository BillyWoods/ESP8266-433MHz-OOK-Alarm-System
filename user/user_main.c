#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"

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
// updates the webpage based on what sensors have been triggered and
//   what mode the alarm is in (armed/disarmed)
void updateWebpage();

packetStack_s unprocessedPackets = { .top = -1 };

/*
    for keeping track (by name) of which sensors have been
    triggered with no duplicate entries, the iter keeps track 
    of the last added element + 1
*/
int triggeredSensorsIter = 0;
char* triggeredSensorsNames[NUM_SENSORS];
uint32_t triggeredSensorsTimestamps[NUM_SENSORS];
void clearTriggeredSensors();
bool alarmArmed = false;
void arm_alarm();
void disarm_alarm();

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

void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
    //nothing
}

void ICACHE_FLASH_ATTR user_init()
{
    // init UART for debugging baud rate comes from user_config.h
    uart_div_modify(0, UART_CLK_FREQ / BAUD_RATE);
    os_printf("\r\nESP8266 OOK decoding\r\n");

    // speed boost (hopefully)
    //system_update_cpu_freq(160);
    system_update_cpu_freq(80);

    // setup loop callback in system task queue
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);

    // init wifi using creds in wifi_config.h. Put your own creds in the file
    connect_wifi(WIFI_SSID, WIFI_PSK);

    // apparently an accurate system time is needed for SSL, so start up
    //   SNTP stuff.
    // SNTP is also used for timestamping emails and alarm triggeringings
    sntp_setservername(0, "time.nist.gov");
    sntp_setservername(1, "time-c.nist.gov");
    sntp_set_timezone(TIMEZONE);
    sntp_init();
}

bool initFinished = false;
void ICACHE_FLASH_ATTR user_init2()
{
    
    // webserver-related initialisation
    init_web_server();
    attach_btn_clear(clearTriggeredSensors);
    attach_btn_arm_alarm(arm_alarm);
    attach_btn_disarm_alarm(disarm_alarm);
    // update so we don't just have a blank page on startup
    updateWebpage(); 

    // email-related stuff
    init_email();

    // init stuff for the ook decoder
    gpio_init();
    gpio_intr_handler_register(ook_intr_handler, (void*) &unprocessedPackets);
    init_ook_decoder();

    os_printf("init finished!\r\n");
    initFinished = true;
}



// needs global declaration for persistence as this pointer
// is handed to webserver.c so shouldn't be cleared at any point
char generatedWebpage[WEBPAGE_SIZE];

// called when the "clear" button is pressed on webpage
void clearTriggeredSensors()
{
    int i;
    for(i = 0; i<triggeredSensorsIter; i++)
    {
        os_free(triggeredSensorsNames[i]);
        os_free(triggeredSensorsTimestamps[i]);
    }
    triggeredSensorsIter = 0;
    //set_webpage(NULL);
    updateWebpage();
}

// called when keyfob transmission is detected or button in webpage is pressed
void arm_alarm()
{
    alarmArmed = true;
    updateWebpage();
}

void disarm_alarm()
{
    alarmArmed = false;
    // clearTriggeredSensors will update the webpage for us
    clearTriggeredSensors();
    //updateWebpage();
}

void updateWebpage()
{ 
    int i;
    char* temp = "<title>ESP8266 Home Monitor</title>\n";

    // display whether the alarm is armed or disarmed
    if (alarmArmed)
        os_sprintf(generatedWebpage, "%s<b>Alarm state: </b>ARMED!<br>\n", temp);
    else
        os_sprintf(generatedWebpage, "%s<b>Alarm state: </b>not armed<br>\n", temp);

    if (triggeredSensorsIter > 0)
    {
        os_sprintf(generatedWebpage, "%s<br><b>Triggered Sensors:</b><br><ol>\n", generatedWebpage);
        for (i = 0; i < triggeredSensorsIter; i++)
        {
            os_sprintf(generatedWebpage, "%s<b><li>%s</b> - triggered at: %s</li>\n", generatedWebpage, 
                       triggeredSensorsNames[i], sntp_get_real_time(triggeredSensorsTimestamps[i]));
        }
        //end the ordered list
        os_sprintf(generatedWebpage, "%s</ol><br>\n", generatedWebpage);
    }
    else
        os_sprintf(generatedWebpage, "%s<br>No triggered sensors\n",generatedWebpage);

    // the clear button
    os_sprintf(generatedWebpage, 
        "%s<br>\n<form action=\"clear\"><input type=\"submit\" value=\"Clear\"></form>", 
        generatedWebpage);
    // the arm button
    os_sprintf(generatedWebpage, 
        "%s\n<form action=\"arm\"><input type=\"submit\" value=\"Arm Alarm\"></form>", 
        generatedWebpage);
    // the disarm button
    os_sprintf(generatedWebpage, 
        "%s\n<form action=\"disarm\"><input type=\"submit\" value=\"Disarm Alarm\"></form>", 
        generatedWebpage);

    set_webpage(generatedWebpage);
}

char generatedEmailBody[EMAIL_LENGTH_MAX];
void generate_email_body()
{
    os_sprintf(generatedEmailBody, "Triggered Sensors:\n");
    int i;
    for (i = 0; i < triggeredSensorsIter; i++)
    {
        os_sprintf(generatedEmailBody, "%s%s- triggered at: %s\n", generatedEmailBody, 
                   triggeredSensorsNames[i], sntp_get_real_time(triggeredSensorsTimestamps[i]));
    }
}

static void ICACHE_FLASH_ATTR loop(os_event_t* events)
{
    if (!initFinished)
    {
        // loop by adding this function to the task queue
        system_os_post(user_procTaskPrio, 0, 0);

        if(is_wifi_connected())
        {
            user_init2();
            initFinished = true;
        }

        return;
    }

    static bool pendingEmailAlert = false;
    // check unprocessed OOK packets to see if there are any newly
    //   triggered sensors
    bool newTriggerings = false;
    while(packets_available(&unprocessedPackets))
    {
        uint32 packet = packet_pop(&unprocessedPackets);
        char* source = my_strdup(ook_ID_to_name(packet));
        // check if the current packet is a new triggering
        bool packetIsNewTriggering = true;

#ifdef PRINT_OOK_PACKETS_DEBUG
        os_printf("%x\r\n", packet);
#endif
        // check for the key fob signals for arming/disarming
        if (packet == ARM_CODE)
            arm_alarm();
        else if (packet == DISARM_CODE)
            disarm_alarm();
        // must be some other code, or invalid
        else if (source != NULL && alarmArmed)
        {
            int i, j;
            // ensure that an already-triggered sensor is not duplicated in list
            for (i = 0; i < triggeredSensorsIter; i++)
            {
                if (triggeredSensorsNames[i] != NULL)
                    for (j = 0; source[j] == triggeredSensorsNames[i][j]; j++)
                    {
                        // reached the end of both strings and they've been the
                        // same all along
                        if (source[j] == '\0')
                            packetIsNewTriggering = false;
                    }
            }
            if (packetIsNewTriggering)
            {
                triggeredSensorsNames[triggeredSensorsIter] = source;
                triggeredSensorsTimestamps[triggeredSensorsIter] = sntp_get_current_timestamp();
                triggeredSensorsIter += 1;
                newTriggerings = true;
            }
            else
                os_free(source);
        }
    }

    if (newTriggerings)
    {
        updateWebpage();
        pendingEmailAlert = true;
    }

    // checked so that a pending email alert is cancelled if the "disarm" button is pressed after a
    //   sensor has been triggered
    if(!alarmArmed)
        pendingEmailAlert = false;
    
    if (pendingEmailAlert)
    {
        if (triggeredSensorsIter > 0 &&
            sntp_get_current_timestamp() - triggeredSensorsTimestamps[0] > ALERT_EMAIL_DELAY)
        {
            generate_email_body();
            send_email("ESP8266 Alarm", generatedEmailBody); 
#ifdef PRINT_EMAIL_DEBUG
            os_printf("trying to send email\n");
#endif
            pendingEmailAlert = false;
        }
    }

    // loop by adding this function to the task queue
    system_os_post(user_procTaskPrio, 0, 0);
}
