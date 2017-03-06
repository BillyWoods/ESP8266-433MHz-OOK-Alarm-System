#ifndef _EMAIL_H
#define _EMAIL_H

#define EMAIL_SEND_TO "recipient@gmail.com"
#define EMAIL_ADDRESS "sending_email@gmail.com"
#define EMAIL_SENDER "ESP8266 alarm"
// auth string generated in python via base64.b64encode(b"\000<email_address>\000<email_password>")
#define EMAIL_AUTH_STRING "base 64 auth string"
#define EMAIL_SERVER "smtp.gmail.com"
#define EMAIL_PORT 465
//const char* cert

void init_email();
void email_debug_func();
bool compose_email(char* subject, char* body);

typedef enum
{
    ERROR      =-1,
    IDLE       = 0,
    DNS_LOOKUP = 1,
    CONNECTING = 2,
    CONNECTED  = 3,
    EHLO       = 4,
    AUTH_PLAIN = 5,
    MAIL_FROM  = 6,
    RCPT_TO    = 7,
    DATA_BEGIN = 8,
    DATA_SEND  = 9,
    QUIT       =10,
    CLOSE_SSL  =11,
    SENT       =12
}SMTP_connec_status;

#endif
