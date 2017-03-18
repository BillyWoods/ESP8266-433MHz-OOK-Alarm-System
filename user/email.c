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
char* emailContents[1000];

SMTP_connec_status currentSMTPStatus;
uint32_t timeOfLastReceive = 0;
bool is_email_timed_out()
{
#ifdef PRINT_EMAIL_DEBUG
    os_printf("time since last email comm: %d", sntp_get_current_timestamp() -  timeOfLastReceive);
#endif
    return sntp_get_current_timestamp() - timeOfLastReceive > EMAIL_TIMEOUT;
}
void increment_SMTP_status()
{
    currentSMTPStatus+=1;
    timeOfLastReceive = sntp_get_current_timestamp();
}

void on_connect_cb(void* arg);
void on_receive_cb(void* arg, char* data, unsigned short dataLen);
void on_sent_cb(void* arg);

void email_ssl_disconnect()
{
    int errorCode = espconn_secure_disconnect(emailServerConn);
#ifdef PRINT_EMAIL_DEBUG
    os_printf("result of disconnect: %d\r\n", errorCode);
#endif
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
#ifdef PRINT_EMAIL_DEBUG
                os_printf("dns lookup failed\r\n");
#endif
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
#ifdef PRINT_EMAIL_DEBUG
            os_printf("got to ehlo\n");
            os_sprintf(smtpCommand, "EHLO %s\r\n\0", EMAIL_SERVER);
            os_printf("%s", smtpCommand);
#endif
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case AUTH_PLAIN:
#ifdef PRINT_EMAIL_DEBUG
            os_printf("got to auth\n");
            os_sprintf(smtpCommand, "AUTH PLAIN %s\r\n\0", EMAIL_AUTH_STRING);
            os_printf("%s", smtpCommand);
#endif
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case MAIL_FROM:
#ifdef PRINT_EMAIL_DEBUG
            os_printf("got to mail from\n");
            os_sprintf(smtpCommand, "MAIL FROM: <%s>\r\n\0", EMAIL_ADDRESS);
            os_printf("%s", smtpCommand);
#endif
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case RCPT_TO:
#ifdef PRINT_EMAIL_DEBUG
            os_printf("got to rcpt to\n");
            os_sprintf(smtpCommand, "RCPT TO: <%s>\r\n\0", EMAIL_SEND_TO);
            os_printf("%s", smtpCommand);
#endif
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case DATA_BEGIN:
#ifdef PRINT_EMAIL_DEBUG
            os_printf("got to data\n");
            os_sprintf(smtpCommand, "DATA\r\n\0");
            os_printf("%s", smtpCommand);
#endif
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case DATA_SEND:
#ifdef PRINT_EMAIL_DEBUG
            os_printf("got to data send\n");
            os_printf("%s", emailContents);
#endif
            errorCode = espconn_secure_send(emailServerConn, (uint8*) emailContents, os_strlen(emailContents));
            break;
        case QUIT:
#ifdef PRINT_EMAIL_DEBUG
            os_printf("got to quit\n");
            os_sprintf(smtpCommand, "QUIT\r\n\0");
            os_printf("%s", smtpCommand);
#endif
            errorCode = espconn_secure_send(emailServerConn, (uint8*) smtpCommand, os_strlen(smtpCommand));
            break;
        case CLOSE_SSL:
            os_printf("email sent!");
            system_os_post(ssl_disconnectPrio, 0, 0);
            increment_SMTP_status();
            break;
        case SENT:
            // do nothing
            return;
    }

    if (errorCode != 0)
    {
#ifdef PRINT_EMAIL_DEBUG
        os_printf("error with smtp! %d %d\r\n", currentSMTPStatus, errorCode);
#endif
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
#ifdef PRINT_EMAIL_DEBUG
        os_printf("DNS lookup failed\r\n");
#endif
        return;
    }

    uint8_t* ipBytes = (uint8_t*) ipaddr;
#ifdef PRINT_EMAIL_DEBUG
    os_printf("%s found at IP: %d.%d.%d.%d\r\n", name, ipBytes[0], ipBytes[1],
              ipBytes[2], ipBytes[3]);
#endif
    os_memcpy(emailServerConn->proto.tcp->remote_ip, ipBytes, 4);

    //uint8_t debugIP[4] = {192,168,0,5};
    //os_memcpy(emailServerConn->proto.tcp->remote_ip, debugIP, 4);
#ifdef PRINT_EMAIL_DEBUG
    print_espconn_config(emailServerConn);
#endif

    currentSMTPStatus = CONNECTING;
    SMTP_client_loop();
}

void on_connect_cb(void* arg)
{
    struct espconn* conn = (struct espconn*) arg;
#ifdef PRINT_EMAIL_DEBUG
    os_printf("connected (cb called!)\n");
#endif
    currentSMTPStatus = CONNECTED;
    SMTP_client_loop();
}

void on_receive_cb(void* arg, char* data, unsigned short dataLen)
{
#ifdef PRINT_EMAIL_DEBUG
    os_printf("received from SMTP serv: %s\n", data);
#endif
    increment_SMTP_status();
    SMTP_client_loop();
}

void on_sent_cb(void* arg)
{
#ifdef PRINT_EMAIL_DEBUG
    os_printf("sent!\r\n");
#endif
}

bool compose_email(char* subject, char* body)
{
    os_sprintf(emailContents, "From: \"%s\" <%s>\nTo: <%s>\nSubject: %s\n\n%s\r\n.\r\n\0",
        EMAIL_SENDER, EMAIL_ADDRESS, EMAIL_SEND_TO, subject, body);
#ifdef PRINT_EMAIL_DEBUG
    os_printf("composed email: %s", emailContents);
#endif
}

void email_debug_func()
{
    send_email("test", "hello world");
}

void send_email(char* subject, char* body)
{
    // can only send one email at a time
    if (currentSMTPStatus == IDLE || currentSMTPStatus == SENT || currentSMTPStatus == ERROR ||
        is_email_timed_out())
    {
        currentSMTPStatus = IDLE;
        increment_SMTP_status();
        compose_email(subject, body);
        SMTP_client_loop();
    }
}

SMTP_connec_status get_email_status()
{
    if (is_email_timed_out())
        currentSMTPStatus = IDLE;
    return currentSMTPStatus;
}
