#include "webserver.h"

void ICACHE_FLASH_ATTR connect_wifi(const char* ssid, const char* password)
{
    struct station_config stationConf;

    wifi_set_opmode(STATION_MODE);
    wifi_station_set_auto_connect(1);
    os_strcpy(&stationConf.ssid, ssid);
    os_strcpy(&stationConf.password, password);
    wifi_station_set_hostname("ESP8266");
    wifi_station_set_config(&stationConf);
    wifi_station_connect();
}

bool is_wifi_connected()
{
    return (wifi_station_get_connect_status() == STATION_GOT_IP);
}

void ICACHE_FLASH_ATTR init_web_server()
{
    struct espconn* HTTPServer = (struct espconn*) os_zalloc(sizeof(struct espconn)); 
    espconn_create(HTTPServer);

    //config webserver for tcp on port 80
    HTTPServer->type = ESPCONN_TCP;
    HTTPServer->state = ESPCONN_NONE;
    HTTPServer->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
    HTTPServer->proto.tcp->local_port = 80;   

    // register our handler to handle tcp traffic on port 80
    espconn_regist_connectcb(HTTPServer, server_handle_new_conn);

    // timeout
    espconn_regist_time(HTTPServer, 15, 0);
    espconn_accept(HTTPServer);
}


char* webpageToServe = NULL;
char* responseHeader =
    "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n";
char* defaultWebpage =
    "<title>ESP8266 Home Monitor</title>Nothing to report at present";

void ICACHE_FLASH_ATTR set_webpage(char* html)
{
    webpageToServe = html;
}


button_pressed_cb on_clear_pressed;

void ICACHE_FLASH_ATTR attach_btn_clear(button_pressed_cb onClearFunc)
{
    on_clear_pressed = onClearFunc;
}

void ICACHE_FLASH_ATTR send_404(struct espconn* sendTo)
{
    char* responseHeader = 
        "HTTP/1.1 404 Not found\r\nConnection: close\r\n\r\n";
    espconn_send(sendTo, (uint8*) responseHeader, strlen(responseHeader));
}

void ICACHE_FLASH_ATTR send_redirect_main_webpage(struct espconn* sendTo)
{
    char* responseHeader = 
        "HTTP/1.1 307 Redirect\r\nLocation: /\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html>Refreshing...</html>\r\n";
    espconn_send(sendTo, (uint8*) responseHeader, strlen(responseHeader));
}

void ICACHE_FLASH_ATTR send_main_webpage(struct espconn* sendTo)
{
    char* htmlData;
    size_t dataLen;
    
    if(webpageToServe != NULL)
    {
        dataLen = strlen(webpageToServe) + strlen(responseHeader);
        htmlData = (char*) os_malloc((dataLen + 1) * sizeof(char));
        os_sprintf(htmlData, "%s%s", responseHeader, webpageToServe);
    }
    else
    {
        dataLen = strlen(defaultWebpage) + strlen(responseHeader);
        htmlData = (char*) os_malloc((dataLen + 1) * sizeof(char));
        os_sprintf(htmlData, "%s%s", responseHeader, defaultWebpage);
    }
    espconn_send(sendTo, (uint8*) htmlData, dataLen);
}

void ICACHE_FLASH_ATTR server_handle_new_conn(void* arg)
{
    // attach receive callback to handle this new connection
    espconn_regist_recvcb((struct espconn*) arg, server_handle_recv_data);
}

// will return a nullptr if no url found, will return pointer
// to a '\0' if url found is simply "/", i.e. root
char* get_requested_url(char* received, size_t receivedLen)
{
    // look for a GET
    int i, j;
    const char* find = "GET /";
    size_t findLen = strlen(find);
    int urlEndPos;
    char* retVal = NULL;

    for (i = 0; i <= receivedLen - findLen; i++)
    {
        bool match = true;

        for (j = 0; j < findLen; j++)
        {
            match = find[j] == received[j + i];
        }
        if (match)
        {
            // read from next char along from match 
            // until space or null terminator encountered
            for (urlEndPos = i + findLen;
                received[urlEndPos] != ' ' && received[urlEndPos] != '\0'; 
                urlEndPos++)
            { /*no actions needed*/ }
            size_t urlLen = urlEndPos - (i + findLen);
            // the combined zero and malloc of memory 1 byte longer
            // than string copied in will work as null terminator
            retVal = (char*) os_zalloc((urlLen + 1) * sizeof(char));
            if (urlLen > 0)
                os_memcpy(retVal, (received+i+findLen), urlLen);
            
            break;
        }
    }
    return retVal;
}

void ICACHE_FLASH_ATTR server_handle_recv_data(void* arg, char* recvData, unsigned short len)
{
    struct espconn* conn = (struct espconn*) arg;
    char* url = get_requested_url(recvData, len);

    // handle different url requests
    
    // invalid GET request sent
    if (url == NULL)
    { 
        send_404(conn);
    }
    // clear button pressed action
    else if ( strcmp(url, "clear?") == 0 ||
              strcmp(url, "clear") == 0 ||
              strcmp(url, "clear/") == 0 )
    {
        on_clear_pressed();
        send_redirect_main_webpage(conn);
    }
    // temporary for testing email
    else if ( strcmp(url, "email_test") == 0)
    {
        os_printf("email debug page accessed\n");
        email_debug_func();
    }
    // main webpage
    else if (*url == '\0')
    {
        send_main_webpage(conn);
    }

    else
    {
        send_404(conn);
    }
    
    // close the connection so browser doesn't hang on "loading"
    espconn_disconnect(conn);

    os_free(url);
}
