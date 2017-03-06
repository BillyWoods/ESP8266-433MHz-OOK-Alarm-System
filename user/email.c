#include "user_interface.h"
#include "sntp.h"
#include "osapi.h"
#include "mem.h"
#include "espconn.h"

#include "email.h"
#include "user_config.h"

/*
    TODO: 

    Implement actual cert checking
*/

extern os_event_t user_procTaskQueue[user_procTaskQueueLen];

struct espconn* emailServerConn;
struct espconn* dnsLookupConn;
char* emailContents[300];
SMTP_connec_status currentSMTPStatus;

void on_connect_cb(void* arg);
void on_receive_cb(void* arg, char* data, unsigned short dataLen);
void on_sent_cb(void* arg);

void email_ssl_disconnect()
{
    int errorCode = espconn_secure_disconnect(emailServerConn);
    os_printf("result of disconnect: %d\r\n", errorCode);
    if (errorCode == 0 && currentSMTPStatus != ERROR)
        currentSMTPStatus = SENT;
    system_os_post(user_procTaskPrio, 0, 0);
}

void init_email()
{
    dnsLookupConn = (struct espconn*) os_zalloc(sizeof(struct espconn));
    espconn_create(dnsLookupConn);
    dnsLookupConn->type = ESPCONN_TCP;
    dnsLookupConn->state = ESPCONN_NONE;
    dnsLookupConn->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
    dnsLookupConn->proto.tcp->remote_port = 53;
    uint8_t dns_server_0[4] = {8,8,8,8};
    os_memcpy(dnsLookupConn->proto.tcp->remote_ip, dns_server_0, 4);
    espconn_dns_setserver(0, (ip_addr_t*)  dns_server_0);

    emailServerConn = (struct espconn*) os_zalloc(sizeof(struct espconn));
    espconn_create(emailServerConn);
    emailServerConn->type = ESPCONN_TCP;
    emailServerConn->state = ESPCONN_NONE;
    emailServerConn->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
    emailServerConn->proto.tcp->remote_port = EMAIL_PORT;
            
    espconn_regist_connectcb(emailServerConn, on_connect_cb);
    espconn_regist_recvcb(emailServerConn, on_receive_cb);
    espconn_regist_sentcb(emailServerConn, on_sent_cb);

    espconn_secure_set_size(ESPCONN_CLIENT, 8192);
    // remove later, note the 0x1 is for SSL client:
    espconn_secure_ca_disable(0x3);
    espconn_secure_cert_req_disable(0x1);
    // espconn_secure_ca_enable(0x1);
    system_os_task(email_ssl_disconnect, ssl_disconnectPrio, user_procTaskQueue, user_procTaskQueueLen);

    // apparently an accurate system time is needed for SSL, so start up
    //   SNTP stuff
    sntp_setservername(0, "time.nist.gov");
    sntp_setservername(1, "time-c.nist.gov");
    sntp_set_timezone(TIMEZONE);
    sntp_init();
}

void print_espconn_config(struct espconn* conn)
{
    os_printf("remote IP: %d.%d.%d.%d\r\n", IP2STR(conn->proto.tcp->remote_ip));
    os_printf("remote port: %d\r\n", conn->proto.tcp->remote_port);
    os_printf("time: %s\r\n", sntp_get_real_time(sntp_get_current_timestamp()));
}

void email_server_dns_lookup_cb(const char *name, ip_addr_t *ipaddr, 
                                void *callback_arg);

void SMTP_client_loop()
{
    // cases below store the value of espconn_* returns in this, has to be defined at start of block due to a quirk in C
    int errorCode = 0;
    switch (currentSMTPStatus)
    {
        char* smtpCommand[100];

        case ERROR:
            // do nothing
            errorCode = -1;
            break;
        case IDLE:
            // do nothing
            break;
        // will require changes to lookup cb before this case works properly 
        case DNS_LOOKUP:
            // perform DNS lookup on host
            errorCode = espconn_gethostbyname(dnsLookupConn, EMAIL_SERVER, (ip_addr_t*) emailServerConn->proto.tcp->remote_ip,
                                  email_server_dns_lookup_cb);
            if (errorCode == ESPCONN_ARG)// || errorCode == ESPCONN_INPROGRESS)
            {
                os_printf("dns lookup failed\r\n");
                currentSMTPStatus = ERROR;
            }
            else
                errorCode = 0;
            break;

        case CONNECTING:
            errorCode = espconn_secure_connect(emailServerConn);
            //os_printf("result of secure espconn: %d\r\n", secureConnectError);

            break;

        case CONNECTED:
            // do nothing as we wait for server to send message on connection
            break;

        case EHLO:
            os_printf("got to ehlo\n");
            os_sprintf(smtpCommand, "EHLO %s\r\n\0", EMAIL_SERVER);
            os_printf("%s", smtpCommand);
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case AUTH_PLAIN:
            os_printf("got to auth\n");
            os_sprintf(smtpCommand, "AUTH PLAIN %s\r\n\0", EMAIL_AUTH_STRING);
            os_printf("%s", smtpCommand);
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case MAIL_FROM:
            os_printf("got to mail from\n");
            os_sprintf(smtpCommand, "MAIL FROM: <%s>\r\n\0", EMAIL_ADDRESS);
            os_printf("%s", smtpCommand);
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case RCPT_TO:
            os_printf("got to rcpt to\n");
            os_sprintf(smtpCommand, "RCPT TO: <%s>\r\n\0", EMAIL_SEND_TO);
            os_printf("%s", smtpCommand);
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case DATA_BEGIN:
            os_printf("got to data\n");
            os_sprintf(smtpCommand, "DATA\r\n\0");
            os_printf("%s", smtpCommand);
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case DATA_SEND:
            os_printf("got to data send\n");
            os_printf("%s", emailContents);
            errorCode = espconn_secure_send(emailServerConn, (uint8*) emailContents, os_strlen(emailContents));
            break;
        case QUIT:
            os_printf("got to quit\n");
            os_sprintf(smtpCommand, "QUIT\r\n\0");
            os_printf("%s", smtpCommand);
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case CLOSE_SSL:
            system_os_post(ssl_disconnectPrio, 0, 0);
            currentSMTPStatus += 1;
            break;
        case SENT:
            // do nothing
            return;
    }

    if (errorCode != 0)
    {
        os_printf("error with smtp! %d %d\r\n", currentSMTPStatus, errorCode);
        currentSMTPStatus = ERROR;
        //system_os_post(ssl_disconnectPrio, 0, 0);
    }

}

void email_server_dns_lookup_cb(const char *name, ip_addr_t *ipaddr, 
                                void *callback_arg)
{
    if (ipaddr == NULL || name == NULL || currentSMTPStatus == ERROR)
    {
        currentSMTPStatus = ERROR;
        os_printf("DNS lookup failed\r\n");
        return;
    }

    uint8_t* ipBytes = (uint8_t*) ipaddr;
    os_printf("%s found at IP: %d.%d.%d.%d\r\n", name, ipBytes[0], ipBytes[1],
              ipBytes[2], ipBytes[3]);
    os_memcpy(emailServerConn->proto.tcp->remote_ip, ipBytes, 4);

    //uint8_t debugIP[4] = {192,168,0,5};
    //os_memcpy(emailServerConn->proto.tcp->remote_ip, debugIP, 4);
    compose_email("test", "hello world");
    print_espconn_config(emailServerConn);

    currentSMTPStatus = CONNECTING;
    SMTP_client_loop();
}

void on_connect_cb(void* arg)
{
    struct espconn* conn = (struct espconn*) arg;
    os_printf("connected (cb called!)\n");
    currentSMTPStatus = CONNECTED;
    SMTP_client_loop();
}

void on_receive_cb(void* arg, char* data, unsigned short dataLen)
{
    os_printf("received from SMTP serv: %s\n", data);
    currentSMTPStatus += 1;
    SMTP_client_loop();
}

void on_sent_cb(void* arg)
{
    os_printf("sent!\r\n");
}

bool compose_email(char* subject, char* body)
{
    os_sprintf(emailContents, "From: \"%s\" <%s>\nTo: <%s>\nSubject: %s\n\n%s\r\n.\r\n\0",
        EMAIL_SENDER, EMAIL_ADDRESS, EMAIL_SEND_TO, subject, body);
    os_printf("composed email: %s", emailContents);
}

void email_debug_func()
{
    // can only send one email at a time
    if (currentSMTPStatus == IDLE || currentSMTPStatus == SENT || currentSMTPStatus == ERROR)
    {
        currentSMTPStatus = DNS_LOOKUP;
        SMTP_client_loop();
    }
}
