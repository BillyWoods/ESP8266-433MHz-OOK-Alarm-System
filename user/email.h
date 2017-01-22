#ifndef _EMAIL_H
#define _EMAIL_H

#define EMAIL_ADDRESS 
#define EMAIL_PASSWORD
#define EMAIL_SERVER 
#define EMAIL_PORT 
//const char* cert

void init_email();
void email_debug_func();
bool send_email(char* subject, char* body);

#endif
