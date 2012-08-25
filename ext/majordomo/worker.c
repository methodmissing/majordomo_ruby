#include "majordomo_ext.h"

VALUE rb_cMajordomoWorker;

/*
 * :nodoc:
 *  GC mark callback
 *
*/
static void rb_mark_majordomo_worker(void *ptr)
{
    rb_majordomo_worker_t *worker = (rb_majordomo_worker_t *)ptr;
    if (worker) {
        rb_gc_mark(worker->broker);
        rb_gc_mark(worker->service);
        rb_gc_mark(worker->heartbeat);
        rb_gc_mark(worker->reconnect);
    }
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_free_majordomo_worker(void *ptr)
{
    rb_majordomo_worker_t *worker = (rb_majordomo_worker_t *)ptr;
    if (worker) {
        if (worker->worker) mdp_worker_destroy(&worker->worker);
        xfree(worker);
        worker = NULL;
    }
}

static VALUE rb_nogvl_mdp_worker_new(void *ptr)
{
    struct nogvl_md_worker_new_args *args = ptr;
    return (VALUE)mdp_worker_new(args->broker, args->service, args->verbose);
}

static VALUE rb_majordomo_worker_s_new(int argc, VALUE *argv, VALUE klass)
{
    rb_majordomo_worker_t *worker = NULL;
    struct nogvl_md_worker_new_args args;
    VALUE obj, broker, service, verbose;
    rb_scan_args(argc, argv, "21", &broker, &service, &verbose);
    if (verbose == Qnil)
        verbose = Qfalse;
    Check_Type(broker, T_STRING);
    Check_Type(service, T_STRING);
    obj = Data_Make_Struct(klass, rb_majordomo_worker_t, rb_mark_majordomo_worker, rb_free_majordomo_worker, worker);

    args.broker = RSTRING_PTR(broker);
    args.service = RSTRING_PTR(service);
    args.verbose = (verbose == Qtrue ? 1 : 0);
    worker->worker = (mdp_worker_t *)rb_thread_blocking_region(rb_nogvl_mdp_worker_new, (void *)&args, RUBY_UBF_IO, 0);
    worker->broker = rb_str_new4(broker);
    worker->service = rb_str_new4(service);
    worker->heartbeat = INT2NUM(MAJORDOMO_WORKER_HEARTBEAT);
    worker->reconnect = INT2NUM(MAJORDOMO_WORKER_RECONNECT);
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

static VALUE rb_majordomo_worker_broker(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->broker;
}

static VALUE rb_majordomo_worker_service(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->service;
}

static VALUE rb_majordomo_worker_heartbeat(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->heartbeat;
}

static VALUE rb_majordomo_worker_reconnect(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->reconnect;
}

static VALUE rb_majordomo_worker_heartbeat_equals(VALUE obj, VALUE heartbeat){
    GetMajordomoWorker(obj);
    Check_Type(heartbeat, T_FIXNUM);
    mdp_worker_set_heartbeat(worker->worker, FIX2INT(heartbeat));
    worker->heartbeat = heartbeat;
    return Qnil;
}

static VALUE rb_majordomo_worker_reconnect_equals(VALUE obj, VALUE reconnect){
    GetMajordomoWorker(obj);
    Check_Type(reconnect, T_FIXNUM);
    mdp_worker_set_reconnect(worker->worker, FIX2INT(reconnect));
    worker->reconnect = reconnect;
    return Qnil;
}

static VALUE rb_majordomo_worker_recv(VALUE obj){
    GetMajordomoWorker(obj);
    zmsg_t *reply = NULL;
    zmsg_t *request = mdp_worker_recv(worker->worker, &reply);
    if (!request)
        return Qnil;
    return MajordomoEncode(rb_str_new2(zmsg_popstr(request)));
}

static VALUE rb_majordomo_worker_close(VALUE obj){
    GetMajordomoWorker(obj);
    mdp_worker_destroy(&worker->worker);
    worker->worker = NULL;
    return Qnil;
}

void _init_majordomo_worker()
{
    rb_cMajordomoWorker = rb_define_class_under(rb_mMajordomo, "Worker", rb_cObject);

    rb_define_singleton_method(rb_cMajordomoWorker, "new", rb_majordomo_worker_s_new, -1);
    rb_define_method(rb_cMajordomoWorker, "broker", rb_majordomo_worker_broker, 0);
    rb_define_method(rb_cMajordomoWorker, "service", rb_majordomo_worker_service, 0);
    rb_define_method(rb_cMajordomoWorker, "heartbeat", rb_majordomo_worker_heartbeat, 0);
    rb_define_method(rb_cMajordomoWorker, "reconnect", rb_majordomo_worker_reconnect, 0);
    rb_define_method(rb_cMajordomoWorker, "heartbeat=", rb_majordomo_worker_heartbeat_equals, 1);
    rb_define_method(rb_cMajordomoWorker, "reconnect=", rb_majordomo_worker_reconnect_equals, 1);
    rb_define_method(rb_cMajordomoWorker, "recv", rb_majordomo_worker_recv, 0);
    rb_define_method(rb_cMajordomoWorker, "close", rb_majordomo_worker_close, 0);
}