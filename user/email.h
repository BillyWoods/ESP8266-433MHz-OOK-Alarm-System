#ifndef _EMAIL_H
#define _EMAIL_H

#define EMAIL_ADDRESS "username"
#define EMAIL_SENDER "ESP8266 alarm"
#define EMAIL_PASSWORD "password"
// auth string generated in python via base64.b64encode(b"\000<email_address>\000<email_password>")
#define EMAIL_AUTH_STRING ""
#define EMAIL_SERVER "smtp.gmail.com"
#define EMAIL_PORT 465
//const char* cert

void init_email();
void email_debug_func();
bool send_email(char* recipient, char* subject, char* body);

typedef enum
{
    ERROR,
    DNS_LOOKUP,
    CONNECTING,
    EHLO,
    AUTH_PLAIN,
    MAIL_FROM,
    RCPT_TO,
    DATA,
    QUIT,
    CLOSE_SSL,
    SENT
}SMTP_connec_status;

#endif
