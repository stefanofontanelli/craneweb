/*
 * craneweb.c -- modern micro web framework for C.
 * (C) 2011 Francesco Romani <fromani at gmail dot com>
 * ZLIB licensed.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
/* FIXME (portability) */
#include <unistd.h>
/* FIXME (portability) */

#include "config.h"

#ifdef ENABLE_BUILTIN_REGEX
#include "regex.h"
#endif
#ifdef ENABLE_BUILTIN_MONGOOSE
#include "mongoose.h"
#endif

#include "craneweb.h"

/*** generic forward declarations ****************************************/
typedef struct crwdispatcher_ CRW_Dispatcher;
/* `router' would be better but it'll clash with `route' */

static int CRW_dispatcher_handle(CRW_Dispatcher *disp, CRW_Request *request,
                                 CRW_Response *response);


typedef struct crwserveradapter_ CRW_ServerAdapter;

static CRW_ServerAdapter *CRW_server_adapter_new(CRW_Instance *inst,
                                                 CRW_ServerAdapterType server_type,
                                                 const CRW_Config *cfg,
                                                 CRW_Dispatcher *disp,
                                                 CRW_LogHandler log);
static void CRW_server_adapter_del(CRW_ServerAdapter *serv);

static CRW_ServerAdapterType CRW_server_adapter_get_type(CRW_ServerAdapter *serv);

static int CRW_server_adapter_is_running(CRW_ServerAdapter *serv);
static int CRW_server_adapter_run(CRW_ServerAdapter *serv);
static int CRW_server_adapter_stop(CRW_ServerAdapter *serv);


/*** instance (0) *********************************************************/
struct crwinstance_ {
    CRW_ServerAdapter *server;
    CRW_ServerAdapterType server_type;
    CRW_Dispatcher *disp;
    CRW_LogHandler log;
    void *log_data;
};

/*** logger **************************************************************/

enum {
    CRW_PANIC_MSG_LEN = 1024,
    CRW_LOG_MSG_LEN = 1024
};

static const char *CRW_log_level_to_str(CRW_LogLevel level)
{
    const char *str = "LOG";
    switch (level) {
      case CRW_LOG_CRITICAL:
        str = "CRI";
        break;
      case CRW_LOG_ERROR:
        str = "ERR";
        break;
      case CRW_LOG_WARNING:
        str = "WRN";
        break;
      case CRW_LOG_INFO:
        str = "INF";
        break;
      case CRW_LOG_DEBUG:
        str = "DBG";
        break;
      default:
        str = "LOG";
        break;
    }
    return str;
}

int CRW_logger_console(void *userdata, CRW_LogLevel level, const char *tag,
                       const char *fmt, va_list args)
{
    char buf[CRW_LOG_MSG_LEN] = { '\0' };
    const char *lvl = CRW_log_level_to_str(level);
    snprintf(buf, sizeof(buf), "[%s] %s %s\n", tag, lvl, fmt);
    vfprintf(stderr, buf, args);
    return 0;
}

static int CRW_log(CRW_Instance *inst,
                   const char *tag, CRW_LogLevel level,
                   const char *fmt, ...)
{
    int err = 0;
    va_list args;

    va_start(args, fmt);
    err = inst->log(inst->log_data, level, tag, fmt, args);
    va_end(args);

    return err;
}

/* When whe call this, we're really in deep trouble here */
static int CRW_panic(const char *tag, const char *fmt, ...)
{
    char buf[CRW_PANIC_MSG_LEN] = { '\0' };
    int err = 0;
    va_list args;

    snprintf(buf, sizeof(buf), "[%s] PANIC %s\n", tag, fmt);

    va_start(args, fmt);
    vfprintf(stderr, buf, args);
    va_end(args);

    return err;
}


/*** instance (1) *********************************************************/

CRW_Instance *CRW_instance_new(CRW_ServerAdapterType server)
{
    CRW_Instance *inst = calloc(1, sizeof(struct crwinstance_));
    if (inst) {
        inst->server = NULL;
        inst->server_type = server;
        inst->disp = "TODO"; /* FIXME */
        inst->log = CRW_logger_console;
        inst->log_data = NULL;
    }
    return inst;
}

void CRW_instance_del(CRW_Instance *inst)
{
    free(inst);
}

/*** request *************************************************************/
struct crwrequest_ {
    /* TODO */
};

/*** response ************************************************************/
struct crwresponse_ {
    /* TODO */
};

CRW_Response *CRW_response_new(CRW_Instance *inst)
{
    return NULL;
}

void CRW_response_del(CRW_Response *res)
{
    free(res);
}

int CRW_response_add_header(CRW_Response *res,
                            const char *name, const char *value)
{
    return -1;
}

int CRW_response_add_body(CRW_Response *res, const char *chunk)
{
    return -1;
}

int CRW_response_send(CRW_Response *res)
{
    return -1;
}


/*** route ***************************************************************/

typedef struct crwkvpair_ CRW_KVPair;
struct crwkvpair_ {
    char *key;
    char *value;
};

static const char *CRW_kvpair_get_by_idx(const CRW_KVPair *pairs, int num,
                                         int idx)
{
    const char *value = NULL;
    if (pairs && num > 0 && idx > 0 && idx < num) {
        value = pairs[idx].value;
    }
    return value;
}

static const char *CRW_kvpair_get_by_tag(const CRW_KVPair *pairs, int num,
                                         const char *tag)
{
    const char *value = NULL;
    if (pairs && num > 0 && tag) {
        int j = 0, found  = 0;
        for (j = 0; !found && j < num; j++) {
            if (!strcmp(tag, pairs[j].key)) {
                value = pairs[j].value;
                found = 1;
            }
        }
    }
    return value;
}


struct crwrouteargs_ {
    CRW_KVPair pairs[CRW_MAX_ROUTE_ARGS];
    int num;
};

int CRW_route_args_count(const CRW_RouteArgs *args)
{
    int num = -1;
    if (args) {
        num = args->num;
    }
    return num;
}

const char *CRW_route_args_get_by_idx(const CRW_RouteArgs *args, int idx)
{
    const char *value = NULL;
    if (args) {
        return CRW_kvpair_get_by_idx(args->pairs, args->num, idx);
    }
    return value;
}

const char *CRW_route_args_get_by_tag(const CRW_RouteArgs *args,
                                      const char *tag)
{
    const char *value = NULL;
    if (args) {
        return CRW_kvpair_get_by_tag(args->pairs, args->num, tag);
    }
    return value;
}

typedef struct crwroute_ CRW_Route;
struct crwroute_ {
    regex_t RE;
    int compiled;
    const char *regex_user;
    char *regex_crane;
    char *regex_tags;
    char *tags[CRW_MAX_ROUTE_ARGS];
    int tag_num;
    regmatch_t matches[CRW_MAX_ROUTE_ARGS];
    int match_num;
    char *data;
};

#ifdef CRW_DEBUG

CRW_PRIVATE
CRW_Route *CRW_route_new(void)
{
    return calloc(1, sizeof(CRW_Route));
}

CRW_PRIVATE
void CRW_route_del(CRW_Route *route)
{
    free(route);
}

CRW_PRIVATE
int CRW_route_tag_count(CRW_Route *route)
{
    return route->tag_num;
}

CRW_PRIVATE
const char *CRW_route_tag_get_by_idx(CRW_Route *route, int idx)
{
    return route->tags[idx];
}

#endif /* CRW_DEBUG */

CRW_PRIVATE
int CRW_route_cleanup(CRW_Route *route)
{
    int err = -1;
    if (route) {
        free((char *)route->regex_user); /* XXX */
        free(route->regex_tags);
        free(route->regex_crane);
        if (route->compiled) {
            regfree(&route->RE);
        }
        free(route);
    }
    return err;
}

CRW_PRIVATE
int CRW_route_scan_regex(CRW_Route *route)
{
    int err = -1;
    if (route) {
        char *rt = route->regex_tags; /* shortcut */
        size_t j = 0, len = strlen(route->regex_user);
        int matching = 0;
        for (j = 0; j < len+1; j++) {
            char c = rt[j];
            if (matching && (c == '\0' || c == '/' || c == ':')) {
                rt[j] = '\0';
                matching  = 0;
            }
            if (c == ':') {
                 if (route->tag_num < CRW_MAX_ROUTE_ARGS) {
                    route->tags[route->tag_num] = &rt[j + 1];
                    matching = 1;
                }
                route->tag_num++;
            }
        }
        err = 0;
    }
    return err;
}

CRW_PRIVATE
int CRW_route_init(CRW_Route *route, const char *regex)
{
    int err = -1;
    if (route && regex) {
        memset(route, 0, sizeof(CRW_Route));
        route->compiled = 0;
        route->regex_user = strdup(regex);
        route->regex_tags = strdup(regex);
        if (route->regex_user && route->regex_tags) {
            err = CRW_route_scan_regex(route);
        } else {
            CRW_panic("rtr", "no memory for route data on [%s]", regex);
            CRW_route_cleanup(route);
            err = -1;
        }
    }
    return err;
}

CRW_PRIVATE
int CRW_route_match(CRW_Route *route, const char *URI)
{
    return 0;
}

CRW_PRIVATE
int CRW_route_fetch(CRW_Route *route, CRW_RouteArgs *args)
{
    return 0;
}

/*** handler *************************************************************/

struct crwhandler_ {
    CRW_Instance *inst;
    CRW_HandlerCallback callback;
    void *userdata;
};

CRW_Handler *CRW_handler_new(CRW_Instance *inst,
                             const char *route,
                             CRW_HandlerCallback callback,
                             void *userdata)
{
    return NULL;
}

void CRW_handler_del(CRW_Handler *handler)
{
    free(handler);
}


int CRW_handler_add_route(CRW_Handler *handler, const char *route)
{
    return -1;
}

/*** server adapters *****************************************************/

enum {
    CRW_PORT_STR_LEN = 8
};

struct crwserveradapter_ {
    CRW_Instance *inst;
    CRW_LogHandler logger;
    CRW_Dispatcher *disp;
    CRW_ServerAdapterType server_type;
    int running;

    void *priv;

    int (*destroy)(CRW_ServerAdapter *serv);
    int (*run)(CRW_ServerAdapter *serv);
    int (*stop)(CRW_ServerAdapter *serv);
};


static int CRW_server_adapter_init_error(CRW_ServerAdapter *serv,
                                         const CRW_Config *cfg)
{
    return -1;
}

/*** server adapters: specific: mongoose *********************************/
#ifdef ENABLE_BUILTIN_MONGOOSE

enum {
    CRW_MONGOOSE_OPTION_NUM = 6
};

typedef struct crwserveradaptermongoose_ CRW_ServerAdapterMongoose;
struct crwserveradaptermongoose_ {
    struct mg_context *ctx;
    char *options[CRW_MONGOOSE_OPTION_NUM + 1]; /* ending NULL */
    char *hostname;
    char *docroot;
    size_t hostlen;
};

static int CRW_server_adapter_mongoose_build(CRW_ServerAdapter *serv,
                                             CRW_Request *req)
{
    return -1;
}


static int CRW_server_adapter_mongoose_send(CRW_ServerAdapter *serv,
                                            CRW_Response *res)
{
    return -1;
}


static void *CRW_mongoose_event_handler(enum mg_event event,
                                        struct mg_connection *conn,
                                        const struct mg_request_info *request_info)
{
    void *processed = "craneweb";
    const char *notyet = "<html><body><h1>NOT YET</h1></body></html>";
    mg_write(conn, notyet, strlen(notyet));
    return processed;

    /* always. Mongoose should'nt do anything on its own */
    CRW_ServerAdapter *serv = request_info->user_data;
    CRW_Request *req = CRW_request_new(serv->inst);
    CRW_Response *res = CRW_response_new(serv->inst);
    if (req && res) {
        int err = 0;
        if (event == MG_NEW_REQUEST) {
            err = CRW_server_adapter_mongoose_build(serv, req);
            err = CRW_dispatcher_handle(serv->disp, req, res);
            if (!err) {
                err = CRW_server_adapter_mongoose_send(serv, res);
                /* if (err) log it */
            } /* else what? FIXME */
            CRW_response_del(res);
            CRW_request_del(req);
        } else if (event == MG_HTTP_ERROR) {
            /* TODO */
        } /* else we're not interested in. */
    } /* else what? FIXME */
    return processed;
}



static int CRW_server_adapter_mongoose_destroy(CRW_ServerAdapter *serv)
{
    int err = -1;
    if (serv) {
        CRW_ServerAdapterMongoose *MG = serv->priv;
        free(MG->hostname);
        free(MG->docroot);
        free(MG);
        err = 0;
    }
    return err;
}


static int CRW_server_adapter_mongoose_run(CRW_ServerAdapter *serv)
{
    int err = -1;
    if (serv) {
        CRW_ServerAdapterMongoose *MG = serv->priv;
        MG->ctx = mg_start(CRW_mongoose_event_handler,
                           serv, /* callback crazyness */
                           MG->options);
        if (MG->ctx) {
            serv->running = 1;
            err = 0;
        } else {
            CRW_log(serv->inst, "mng", CRW_LOG_CRITICAL,
                    "server run mg_start() failed");
        }
    } else {
        CRW_panic("mng", "missing server instance to run");
    }
    return err;
}

static int CRW_server_adapter_mongoose_stop(CRW_ServerAdapter *serv)
{
    int err = -1;
    if (serv) {
        CRW_ServerAdapterMongoose *MG = serv->priv;
        mg_stop(MG->ctx);
        serv->running = 0;
    }
    return err;
}

static int CRW_server_adapter_init_mongoose(CRW_ServerAdapter *serv,
                                            const CRW_Config *cfg)
{
    int err = -1;
    if (serv && cfg) {
        CRW_ServerAdapterMongoose *MG = calloc(1, sizeof(CRW_ServerAdapterMongoose));
        if (MG) {
            /* TODO: parameters validation */
            MG->hostlen = strlen(cfg->host);
            MG->hostlen += 1 + CRW_PORT_STR_LEN + 1;
            MG->hostname = calloc(1, MG->hostlen);
            snprintf(MG->hostname, MG->hostlen,
                     "%s:%i", cfg->host, cfg->port);
            CRW_log(serv->inst, "mng", CRW_LOG_DEBUG,
                    "will listen on (%s)", MG->hostname);
            MG->docroot = strdup(cfg->document_root);
            CRW_log(serv->inst, "mng", CRW_LOG_DEBUG,
                    "will serve from (%s)", MG->docroot);
            if (MG->hostname && MG->docroot) {
                MG->options[0] = "document_root";
                MG->options[1] = MG->docroot;
                MG->options[2] = "listening_ports";
                MG->options[3] = MG->hostname;
                MG->options[4] = "num_threads";
                MG->options[5] = "1";
                MG->options[6] = NULL;
                serv->destroy  = CRW_server_adapter_mongoose_destroy;
                serv->run      = CRW_server_adapter_mongoose_run;
                serv->stop     = CRW_server_adapter_mongoose_stop;
                serv->priv     = MG;
                err = 0;
            } else {
                CRW_log(serv->inst, "mng", CRW_LOG_CRITICAL,
                        "no memory for server context parameters");
                CRW_server_adapter_mongoose_destroy(serv);
                err = -1;
            }
        } else {
            CRW_log(serv->inst, "mng", CRW_LOG_CRITICAL,
                    "no memory for server context");
        }
    } else {
        CRW_panic("mng", "missing server instance to intialize");
    }
    return err;
}
#else /* ENABLE_BUILTIN_MONGOOSE */

static int CRW_server_adapter_init_mongoose(CRW_ServerAdapter *serv,
                                            const CRW_Config *cfg)
{
    /* log it */
    return -1;
}

#endif /* ENABLE_BUILTIN_MONGOOSE */


/*** server adapters: generics *******************************************/

static const char *CRW_server_type_to_str(CRW_ServerAdapterType server_type)
{
    const char *str = "unknown";
    switch (server_type) {
      case CRW_SERVER_ADAPTER_NONE:
        str = "error";
        break;
      case CRW_SERVER_ADAPTER_MONGOOSE: /* fallback */
      case CRW_SERVER_ADAPTER_DEFAULT:
        str = "mongoose";
        break;
      default:
        str = "unknown";
        break;
    }
    return str;
}

typedef struct crwserveradapterdesc_ CRW_ServerAdapterDesc;
struct crwserveradapterdesc_ {
    CRW_ServerAdapterType server_type;
    int (*server_init)(CRW_ServerAdapter *serv, const CRW_Config *cfg);
};

static CRW_ServerAdapterDesc CRW_adapters[] = {
    { CRW_SERVER_ADAPTER_MONGOOSE, CRW_server_adapter_init_mongoose },
    { CRW_SERVER_ADAPTER_DEFAULT,  CRW_server_adapter_init_mongoose },
    { CRW_SERVER_ADAPTER_NONE,     CRW_server_adapter_init_error    }
};

/* this is the funniest chunk of code I ever wrote */
static int CRW_server_adapter_init(CRW_ServerAdapter *serv,
                                   const CRW_Config *cfg)
{
    int err = -1;
    if (serv) {
        const char *serv_str = CRW_server_type_to_str(serv->server_type);
        int j = 0, found = 0;
        for (j = 0;
             !found && CRW_adapters[j].server_type != CRW_SERVER_ADAPTER_NONE;
             j++) {
            if (CRW_adapters[j].server_type == serv->server_type) {
                CRW_log(serv->inst, "adp", CRW_LOG_DEBUG,
                        "found initializer for server [%s]",
                        serv_str);
                err = CRW_adapters[j].server_init(serv, cfg);
                found = 1;
            }
        }
        if (!found) {
            CRW_log(serv->inst, "adp", CRW_LOG_CRITICAL,
                    "missing initializer for server [%s]",
                    serv_str);
        }
    } else {
        CRW_panic("adp", "missing server instance to intialize");
    }
    return err;
}

static CRW_ServerAdapter *CRW_server_adapter_new(CRW_Instance *inst,
                                                 CRW_ServerAdapterType server_type,
                                                 const CRW_Config *cfg,
                                                 CRW_Dispatcher *disp,
                                                 CRW_LogHandler log)
{
    CRW_ServerAdapter *serv = NULL;
    if (cfg && disp && log) {
        serv = calloc(1, sizeof(struct crwserveradapter_));
        if (serv) {
            int err = 0;

            serv->inst = inst;
            serv->logger = log;
            serv->disp = disp;
            serv->server_type = server_type;
            serv->running = 0;

            CRW_log(inst, "adp", CRW_LOG_DEBUG, "new server [%s]",
                    CRW_server_type_to_str(serv->server_type));
            err = CRW_server_adapter_init(serv, cfg);
            if (err) {
                free(serv);
                serv = NULL;
            }
        } else {
            CRW_panic("adp", "no memory for server adapter");
        }
    } else {
        CRW_panic("adp", "missing parameters");
    }
    return serv;
}

static void CRW_server_adapter_del(CRW_ServerAdapter *serv)
{
    if (serv) {
        int err = serv->destroy(serv);
        if (!err) {
            free(serv);
        } /* else log it */
    }
    return;
}

static CRW_ServerAdapterType CRW_server_adapter_get_type(CRW_ServerAdapter *serv)
{
    CRW_ServerAdapterType server_type = CRW_SERVER_ADAPTER_DEFAULT; /* meh */
    if (serv) {
        server_type = serv->server_type;
    }
    return server_type;
}

static int CRW_server_adapter_is_running(CRW_ServerAdapter *serv)
{
    int running = 0;
    if (serv) {
        running = serv->running;
    }
    return running;
}

static int CRW_server_adapter_run(CRW_ServerAdapter *serv)
{
    int err = -1;
    if (serv) {
        err = serv->run(serv);
    }
    return err;
}

static int CRW_server_adapter_stop(CRW_ServerAdapter *serv)
{
    int err = -1;
    if (serv) {
        err = serv->stop(serv);
    }
    return err;
}



/*** instance (2) ********************************************************/

int CRW_instance_set_logger(CRW_Instance *inst, CRW_LogHandler logger)
{
    int err = -1;
    if (inst && logger) {
        inst->log = logger;
        err = 0;
    }
    return err;
}

int CRW_instance_add_handler(CRW_Instance *inst, CRW_Handler *handler)
{
    return -1;
}

/*** runtime(!) **********************************************************/

static void CRW_wait(CRW_Instance *instance)
{
    while (instance
        && CRW_server_adapter_is_running(instance->server)) {
        /* FIXME (portability) */
        sleep(1);
    }
    return;
}

int CRW_run(CRW_Instance *instance, const CRW_Config *cfg)
{
    int err = -1;
    if (instance && cfg) {
        instance->server = CRW_server_adapter_new(instance,
                                                  instance->server_type,
                                                  cfg,
                                                  instance->disp,
                                                  instance->log);
        if (instance->server) {
            err = CRW_server_adapter_run(instance->server);
            if (!err) {
                CRW_wait(instance);
            } else {
                CRW_log(instance, "run", CRW_LOG_CRITICAL,
                        "server run failed with error = [%i]", err);
            }
        } else {
            CRW_log(instance, "run", CRW_LOG_CRITICAL,
                    "no server to run");
        }
    } else {
        CRW_panic("run", "missing instance or configuration");
    }
    return err;
}

/* vim: set ts=4 sw=4 et */
/* EOF!*/

