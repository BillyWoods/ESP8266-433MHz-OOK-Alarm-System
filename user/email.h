#ifndef _EMAIL_H
#define _EMAIL_H

#define EMAIL_SEND_TO "recipient@example.com"
#define EMAIL_ADDRESS "sending_account@gmail.com"
#define EMAIL_SENDER "ESP8266 alarm"
// auth string generated in python via base64.b64encode(b"\000<email_address>\000<email_password>")
#define EMAIL_AUTH_STRING "sdfseaERQQFDDSE="
#define EMAIL_SERVER "smtp.gmail.com"
#define EMAIL_PORT 465
#define EMAIL_TIMEOUT 20
#define EMAIL_LENGTH_MAX 1000

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

void init_email();
void email_debug_func();
void send_email(char* subject, char* body);
SMTP_connec_status get_email_status();

#endif
