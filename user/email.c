#include "user_interface.h"
#include "sntp.h"
#include "osapi.h"
#include "mem.h"
#include "espconn.h"

#include "email.h"
#include "user_config.h"

/*
    TODO: 

    start talking SMTP to connected server

    Implement actual cert checking
*/

extern os_event_t user_procTaskQueue[user_procTaskQueueLen];

struct espconn* emailServerConn;
char* emailContents[500];
SMTP_connec_status currentSMTPStatus;

void email_ssl_disconnect()
{
    int errorCode = espconn_secure_disconnect(emailServerConn);
    os_printf("result of disconnect: %d\r\n", errorCode);
    if (errorCode)
        currentSMTPStatus = ERROR;
    else
        currentSMTPStatus = SENT;
    system_os_post(user_procTaskPrio, 0, 0);
}

void init_email()
{
    emailServerConn = (struct espconn*) os_zalloc(sizeof(struct espconn));
    espconn_create(emailServerConn);
    emailServerConn->type = ESPCONN_TCP;
    emailServerConn->state = ESPCONN_NONE;
    emailServerConn->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
    emailServerConn->proto.tcp->remote_port = EMAIL_PORT;

    espconn_secure_set_size(ESPCONN_CLIENT, 8000);
    // remove later, note the 0x1 is for SSL client:
    espconn_secure_ca_disable(0x3);
    // espconn_secure_ca_enable(0x1);
    system_os_task(email_ssl_disconnect, ssl_disconnectPrio, user_procTaskQueue, user_procTaskQueueLen);

    // apparently an accurate system time is needed for SSL, so start up
    //   SNTP stuff
    sntp_setservername(0, "time.nist.gov");
    sntp_setservername(1, "time-c.nist.gov");
    sntp_set_timezone(TIMEZONE);
    sntp_init();
}

void email_server_dns_lookup_cb(const char *name, ip_addr_t *ipaddr, 
                                void *callback_arg)
{
    if (ipaddr == NULL || name == NULL)
    {
        currentSMTPStatus = ERROR;
        os_printf("DNS lookup failed");
        return;
    }

    uint8_t* ipBytes = (uint8_t*) ipaddr;
    os_printf("%s found at IP: %d.%d.%d.%d\r\n", name, ipBytes[0], ipBytes[1],
              ipBytes[2], ipBytes[3]);
    os_memcpy(emailServerConn->proto.tcp->remote_ip, ipBytes, 4);

    //uint8_t debugIP[4] = {192,168,0,5};
    //os_memcpy(emailServerConn->proto.tcp->remote_ip, debugIP, 4);
    send_email("billy.woods109@gmail.com", "test", "hello world");
}

void on_connect_cb(void* arg);
void on_receive_cb(void* arg, char* data, unsigned short dataLen);

void SMTP_client_loop()
{
    switch (currentSMTPStatus)
    {
        case ERROR:
            // do nothing
            return;
        // will require changes to lookup cb before this case works properly 
        case DNS_LOOKUP:
            espconn_gethostbyname(emailServerConn, EMAIL_SERVER, (ip_addr_t*) emailServerConn->proto.tcp->remote_ip,
                                  email_server_dns_lookup_cb);
            break;

        case CONNECTING:
            espconn_regist_connectcb(emailServerConn, on_connect_cb);
            espconn_regist_recvcb(emailServerConn, on_receive_cb);
            int secureConnectError = espconn_secure_connect(emailServerConn);
            //os_printf("result of secure espconn: %d\r\n", secureConnectError);

            if (secureConnectError)
                currentSMTPStatus = ERROR;
            break;

        case EHLO:
            os_printf("got to ehlo\n");
            break;
        case AUTH_PLAIN:
            break;
        case MAIL_FROM:
            break;
        case RCPT_TO:
            break;
        case DATA:
            break;
        case QUIT:
            break;
        case CLOSE_SSL:
            system_os_post(ssl_disconnectPrio, 0, 0);
            break;
        case SENT:
            // do nothing
            return;
    }

}

void on_connect_cb(void* arg)
{
    struct espconn* conn = (struct espconn*) arg;
    os_printf("connected (cb called!)");
    currentSMTPStatus = EHLO;
    SMTP_client_loop();
}

void on_receive_cb(void* arg, char* data, unsigned short dataLen)
{
    os_printf("received from SMTP serv: %s\n", data);
    currentSMTPStatus += 1;
    SMTP_client_loop();
}

void print_espconn_config(struct espconn* conn)
{
    os_printf("remote IP: %d.%d.%d.%d\r\n", IP2STR(conn->proto.tcp->remote_ip));
    os_printf("remote port: %d\r\n", conn->proto.tcp->remote_port);
}

bool send_email(char* recipient, char* subject, char* body)
{
    os_sprintf(emailContents, "From: \"%s\" <%s>\nTo: <%s>\nSubject: %s\n\n%s\n.\r\n\r\n",
        EMAIL_SENDER, EMAIL_ADDRESS, recipient, subject, body);
    os_printf(emailContents);
    os_printf("connect cb registered? %d\r\n", espconn_regist_connectcb(emailServerConn, on_connect_cb));
    int secureConnectError = espconn_secure_connect(emailServerConn);
    print_espconn_config(emailServerConn);
    os_printf("result of secure espconn: %d\r\n", secureConnectError);

    if (secureConnectError)
        currentSMTPStatus = ERROR;
    else
        currentSMTPStatus = CONNECTING;
}

void email_debug_func()
{
    ip_addr_t emailServerIP;
    // perform DNS lookup on host
    os_printf("dns lookup: %d\n", espconn_gethostbyname(emailServerConn, EMAIL_SERVER, (ip_addr_t*) emailServerConn->proto.tcp->remote_ip,
                          email_server_dns_lookup_cb));
    currentSMTPStatus = DNS_LOOKUP;
}
