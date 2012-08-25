#include "majordomo_ext.h"

VALUE rb_cMajordomoClient;

/*
 * :nodoc:
 *  GC mark callback
 *
*/
static void rb_mark_majordomo_client(void *ptr)
{
    rb_majordomo_client_t *client = (rb_majordomo_client_t *)ptr;
    if (client) {
        rb_gc_mark(client->broker);
        rb_gc_mark(client->timeout);
        rb_gc_mark(client->retries);
    }
}

static VALUE rb_nogvl_mdp_client_close(void *ptr)
{
    mdp_client_t *client = ptr;
    mdp_client_destroy(&client);
    return Qnil;
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_free_majordomo_client(void *ptr)
{
    rb_majordomo_client_t *client = (rb_majordomo_client_t *)ptr;
    if (client) {
        if (client->client) rb_thread_blocking_region(rb_nogvl_mdp_client_close, (void *)client->client, RUBY_UBF_IO, 0);
        xfree(client);
        client = NULL;
    }
}

static VALUE rb_nogvl_mdp_client_new(void *ptr)
{
    struct nogvl_md_client_new_args *args = ptr;
    return (VALUE)mdp_client_new(args->broker, args->verbose);
}

static VALUE rb_majordomo_client_s_new(int argc, VALUE *argv, VALUE klass)
{
    rb_majordomo_client_t *client = NULL;
    struct nogvl_md_client_new_args args;
    VALUE obj, broker, verbose;
    rb_scan_args(argc, argv, "11", &broker, &verbose);
    if (verbose == Qnil)
        verbose = Qfalse;
    Check_Type(broker, T_STRING);
    obj = Data_Make_Struct(klass, rb_majordomo_client_t, rb_mark_majordomo_client, rb_free_majordomo_client, client);

    args.broker = RSTRING_PTR(broker);
    args.verbose = (verbose == Qtrue ? 1 : 0);
    client->client = (mdp_client_t *)rb_thread_blocking_region(rb_nogvl_mdp_client_new, (void *)&args, RUBY_UBF_IO, 0);
    client->broker = rb_str_new4(broker);
    client->timeout = INT2NUM(MAJORDOMO_CLIENT_TIMEOUT);
    client->retries = INT2NUM(MAJORDOMO_CLIENT_RETRIES);
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

static VALUE rb_majordomo_client_broker(VALUE obj){
    GetMajordomoClient(obj);
    return client->broker;
}

static VALUE rb_majordomo_client_timeout(VALUE obj){
    GetMajordomoClient(obj);
    return client->timeout;
}

static VALUE rb_majordomo_client_retries(VALUE obj){
    GetMajordomoClient(obj);
    return client->retries;
}

static VALUE rb_majordomo_client_timeout_equals(VALUE obj, VALUE timeout){
    GetMajordomoClient(obj);
    Check_Type(timeout, T_FIXNUM);
    mdp_client_set_timeout(client->client, FIX2INT(timeout));
    client->timeout = timeout;
    return Qnil;
}

static VALUE rb_majordomo_client_retries_equals(VALUE obj, VALUE retries){
    GetMajordomoClient(obj);
    Check_Type(retries, T_FIXNUM);
    mdp_client_set_retries(client->client, FIX2INT(retries));
    client->retries = retries;
    return Qnil;
}

static VALUE rb_nogvl_mdp_client_send(void *ptr)
{
    struct nogvl_md_client_send_args *args = ptr;
    return (VALUE)mdp_client_send(args->client, args->service, &args->request);
}

static VALUE rb_majordomo_client_send(VALUE obj, VALUE service, VALUE message){
    zmsg_t *request = NULL;
    zmsg_t *reply = NULL;
    struct nogvl_md_client_send_args args;
    GetMajordomoClient(obj);
    Check_Type(service, T_STRING);
    Check_Type(message, T_STRING);
    request = zmsg_new();
    if (!request)
        return Qnil;
    if (zmsg_pushstr(request, RSTRING_PTR(message)) != 0){
        zmsg_destroy(&request);
        return Qnil;
    }
    args.client = client->client;
    args.service = RSTRING_PTR(service);
    args.request = request;
    reply = (zmsg_t *)rb_thread_blocking_region(rb_nogvl_mdp_client_send, (void *)&args, RUBY_UBF_IO, 0);
    if (!reply)
        return Qnil;
    return MajordomoEncode(rb_str_new2(zmsg_popstr(reply)));
}

static VALUE rb_majordomo_client_close(VALUE obj){
    VALUE ret;
    GetMajordomoClient(obj);
    ret = rb_thread_blocking_region(rb_nogvl_mdp_client_close, (void *)client->client, RUBY_UBF_IO, 0);
    client->client = NULL;
    return ret;
}

void _init_majordomo_client()
{
    rb_cMajordomoClient = rb_define_class_under(rb_mMajordomo, "Client", rb_cObject);

    rb_define_singleton_method(rb_cMajordomoClient, "new", rb_majordomo_client_s_new, -1);
    rb_define_method(rb_cMajordomoClient, "broker", rb_majordomo_client_broker, 0);
    rb_define_method(rb_cMajordomoClient, "timeout", rb_majordomo_client_timeout, 0);
    rb_define_method(rb_cMajordomoClient, "retries", rb_majordomo_client_retries, 0);
    rb_define_method(rb_cMajordomoClient, "timeout=", rb_majordomo_client_timeout_equals, 1);
    rb_define_method(rb_cMajordomoClient, "retries=", rb_majordomo_client_retries_equals, 1);
    rb_define_method(rb_cMajordomoClient, "send", rb_majordomo_client_send, 2);
    rb_define_method(rb_cMajordomoClient, "close", rb_majordomo_client_close, 0);
}