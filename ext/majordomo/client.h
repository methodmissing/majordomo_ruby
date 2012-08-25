#ifndef MAJORDOMO_CLIENT_H
#define MAJORDOMO_CLIENT_H

typedef struct {
    mdp_client_t *client;
    VALUE broker;
    VALUE timeout;
    VALUE retries;
} rb_majordomo_client_t;

#define MAJORDOMO_CLIENT_TIMEOUT 2500
#define MAJORDOMO_CLIENT_RETRIES 3

#define GetMajordomoClient(obj) \
    rb_majordomo_client_t *client = NULL; \
    Data_Get_Struct(obj, rb_majordomo_client_t, client); \
    if (!client) rb_raise(rb_eTypeError, "uninitialized Majordomo client!"); \
    if (!client->client) rb_raise(rb_eRuntimeError, "Majordomo client has already been closed!");

struct nogvl_md_client_new_args {
    char *broker;
    int verbose;
};

void _init_majordomo_client();

#endif