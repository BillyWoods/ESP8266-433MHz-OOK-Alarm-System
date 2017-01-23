#include "user_interface.h"
#include "sntp.h"
#include "osapi.h"
#include "mem.h"
#include "espconn.h"

#include "email.h"

/*
    TODO: 
    
    see why dns lookup only seems to work once 
    
    check what value original IP pointer takes.

    move system_os_post/system_os_task stuff into its own header
    to clean it up since it is now used both in main and in here.

    Implement actual cert checking
*/

extern os_event_t user_procTaskQueue[2];
struct espconn* emailServerConn;

void email_ssl_disconnect()
{
    int result = espconn_secure_disconnect(emailServerConn);
    os_printf("result of disconnect: %d\r\n", result);
    system_os_post(0, 0, 0);
}

void init_email()
{
    emailServerConn = (struct espconn*) os_zalloc(sizeof(struct espconn));
    espconn_create(emailServerConn);
    emailServerConn->type = ESPCONN_TCP;
    emailServerConn->state = ESPCONN_NONE;
    emailServerConn->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
    emailServerConn->proto.tcp->remote_port = EMAIL_PORT;

    // remove later, note the 0x1 is for SSL client:
    espconn_secure_ca_disable(0x3);
    // espconn_secure_ca_enable(0x1);
    system_os_task(email_ssl_disconnect, 1, user_procTaskQueue, 2);

    // apparently an accurate system time is needed for SSL, so start up
    //   SNTP stuff
    sntp_setservername(0, "time.nist.gov");
    sntp_setservername(1, "time-c.nist.gov");
    sntp_set_timezone(11);
    sntp_init();
}

void email_server_dns_lookup_cb(const char *name, ip_addr_t *ipaddr, 
                                void *callback_arg)
{
    if (ipaddr == NULL || name == NULL)
        return;

    uint8_t* ipBytes = (uint8_t*) ipaddr;
    os_printf("%s found at IP: %d.%d.%d.%d\r\n", name, ipBytes[0], ipBytes[1],
              ipBytes[2], ipBytes[3]);
    //os_memcpy(emailServerConn->proto.tcp->remote_ip, ipBytes, 4);
    send_email("hi", "testing testing 123");
}

bool send_email(char* subject, char* body)
{
    int result1 = espconn_secure_connect(emailServerConn);
    os_printf("result of secure espconn: %d\r\n", result1);
    system_os_post(1, 0, 0);
}

void email_debug_func()
{
    ip_addr_t emailServerIP;
    // perform DNS lookup on host
    espconn_gethostbyname(emailServerConn, EMAIL_SERVER, (ip_addr_t*) emailServerConn->proto.tcp->remote_ip,
                          email_server_dns_lookup_cb);
}
