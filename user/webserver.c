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
char* defaultWebpage =
    "<b>Nothing to see here</b><br>Default webpage";

void ICACHE_FLASH_ATTR set_webpage(char* html)
{
    webpageToServe = html;
}

void ICACHE_FLASH_ATTR server_handle_new_conn(void* arg)
{
    struct espconn* conn = (struct espconn*) arg;
    char* htmlData;
    size_t dataLen;
    espconn_regist_recvcb(conn, server_handle_recv_data);
    
    // assume it was a GET request and serve some data
    if(webpageToServe != NULL)
    {
        dataLen = strlen(webpageToServe);
        htmlData = webpageToServe;
    }
    else
    {
        dataLen = strlen(defaultWebpage);
        htmlData = defaultWebpage;
    }
    espconn_send(conn, (uint8*) htmlData, dataLen);
}


void ICACHE_FLASH_ATTR server_handle_recv_data(void* arg, char* recvData, unsigned short len)
{
    struct espconn* conn = (struct espconn*) arg;
    os_printf("sent to this device:\r\n%s", recvData);
}
