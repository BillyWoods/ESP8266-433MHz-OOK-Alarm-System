#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

#include "email.h"

struct espconn* email_server_conn;
void init_email()
{
    email_server_conn = (struct espconn*) os_zalloc(sizeof(struct espconn));
    return;
}

void email_debug_func()
{
    return;
}
