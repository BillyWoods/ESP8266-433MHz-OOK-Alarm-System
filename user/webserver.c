#include "webserver.h"

void ICACHE_FLASH_ATTR connect_wifi(char* ssid, char* password)
{
    struct station_config stationConf;

    wifi_set_opmode(STATION_MODE);
    os_strcpy(&(stationConf.ssid), ssid);
    os_strcpy(&(stationConf.password), password);
    //wifi_station_set_hostname("whatever");
    wifi_station_set_config(&stationConf);
    wifi_station_connect();
}

bool is_wifi_connected()
{
    return (wifi_station_get_connect_status() == STATION_GOT_IP);
}

void ICACHE_FLASH_ATTR init_web_server()
{

}
