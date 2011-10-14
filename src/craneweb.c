/*
 * craneweb.c -- modern micro web framework for C.
 * (C) 2011 Francesco Romani <fromani at gmail dot com>
 * ZLIB licensed.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <sys/types.h>
/* FIXME (portability) */
#include <unistd.h>
/* FIXME (portability) */

#include "config.h"

#include "list.h"
#include "stringbuilder.h"
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

static CRW_Dispatcher *CRW_dispatcher_new(CRW_Instance *inst);
static void CRW_dispatcher_del(CRW_Dispatcher *disp);

static int CRW_dispatcher_init(CRW_Dispatcher *disp, CRW_Instance *inst);
static int CRW_dispatcher_fini(CRW_Dispatcher *disp);

static CRW_Response *CRW_dispatcher_handle(CRW_Dispatcher *disp,
                                           CRW_Request *request);

static CRW_Response *CRW_handler_call(CRW_Handler *handler,
                                      const CRW_RouteArgs *args,
                                      const CRW_Request *req);

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

/*** KV pairs*************************************************************/

typedef struct crwkvpair_ CRW_KVPair;
struct crwkvpair_ {
    const char *key;
    const char *value;
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

static const char *CRW_kvpair_get_by_key(const CRW_KVPair *pairs, int num,
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

static const char *CRW_kvpair_get_by_ikey(const CRW_KVPair *pairs, int num,
                                          const char *tag)
{
    const char *value = NULL;
    if (pairs && num > 0 && tag) {
        int j = 0, found  = 0;
        for (j = 0; !found && j < num; j++) {
            if (!strcasecmp(tag, pairs[j].key)) {
                value = pairs[j].value;
                found = 1;
            }
        }
    }
    return value;
}


/*** instance (1) *********************************************************/

CRW_Instance *CRW_instance_new(CRW_ServerAdapterType server)
{
    CRW_Instance *inst = calloc(1, sizeof(struct crwinstance_));
    if (inst) {
        inst->server = NULL;
        inst->server_type = server;
        inst->disp = CRW_dispatcher_new(inst);
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
    CRW_RequestMethod method;
    const char *URI;
    const char *query_string;
    CRW_KVPair headers[CRW_MAX_REQUEST_HEADERS];
    int num_headers;
    /* shortcut & goodies */
    int is_xhr;
};

CRW_PRIVATE
CRW_Request *CRW_request_new(CRW_Instance *inst)
{
    CRW_Request *req = NULL;
    if (inst) {
        req = calloc(1, sizeof(CRW_Request));
    }
    return req;
}

CRW_PRIVATE
void CRW_request_del(CRW_Request *req)
{
    free(req);
}

CRW_RequestMethod CRW_request_get_method(const CRW_Request *req)
{
    CRW_RequestMethod meth = CRW_REQUEST_METHOD_UNSUPPORTED;
    if (req) {
        meth = req->method;
    }
    return meth;
}

int CRW_request_is_xhr(const CRW_Request *req)
{
    int is_xhr = 0;
    if (req) {
        is_xhr = req->is_xhr;
    }
    return is_xhr;
}


const char *CRW_request_get_header_value(const CRW_Request *req, const char *header)
{
    const char *value = NULL;
    if (req && header) {
        value = CRW_kvpair_get_by_ikey(req->headers, req->num_headers, header);
    }
    return value;
}

int CRW_request_count_headers(const CRW_Request *req)
{
    int num = -1;
    if (req) {
        num = req->num_headers;
    }
    return num;
}

int CRW_request_get_header_by_idx(const CRW_Request *req, int idx,
                                  const char **header, const char **value)
{
    int err = -1;
    if (req && header && value) {
        if (idx < 0 || idx >= req->num_headers) {
            err = 1;
        } else {
            *header = req->headers[idx].key;
            *value  = req->headers[idx].value;
            err = 0;
        }
    }
    return err;
}


/*** response ************************************************************/

enum {
    CRW_RESPONSE_DEFAULT_BODY_LEN = 1024
};

struct crwresponse_ {
    int status_code;
    list headers;
    stringbuilder *body;
};

CRW_Response *CRW_response_new(CRW_Instance *inst)
{
    CRW_Response *res = NULL;
    res = calloc(1, sizeof(CRW_Response));
    if (res) {
        res->status_code = 200; /* FIXME */
        list_init(&res->headers, free);
        res->body = sb_new_with_size(CRW_RESPONSE_DEFAULT_BODY_LEN);
        if (!res->body) {
            free(res);
            res = NULL;
        }
    }
    return res;
}

void CRW_response_del(CRW_Response *res)
{
    if (res) {
        list_destroy(&res->headers);
        sb_destroy(res->body, 1);
    }
    free(res);
}

int CRW_response_add_header(CRW_Response *res,
                            const char *name, const char *value)
{
    int err = -1;
    if (res && name) {
        char hdrbuf[CRW_RESPONSE_DEFAULT_BODY_LEN] = { '\0' };
        char *pc = NULL;
        snprintf(hdrbuf, sizeof(hdrbuf), "%s%s%s\r\n",
                 name, (value) ?":" :"", value);
        pc = strdup(hdrbuf);
        if (pc) {
            list_element *tail = list_tail(&(res->headers));
            err = list_insert_next(&res->headers, tail, pc);
        } else {
            err = 1;
        }
    }
    return err;
}

int CRW_response_add_body(CRW_Response *res, const char *chunk)
{
    int err = -1;
    if (res && chunk) {
        sb_append_str(res->body, chunk);
        err = 0;
    }
    return err;
}


/*** route ***************************************************************/

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
        return CRW_kvpair_get_by_key(args->pairs, args->num, tag);
    }
    return value;
}

#define SUBEXPR_STR "([[:print:]]*)"
#define SUBEXPR_LEN 14

typedef struct crwroute_ CRW_Route;
struct crwroute_ {
    regex_t RE;
    int compiled;
    const char *regex_user;
    char *regex_crane;
    char *regex_tags;
    const char *tags[CRW_MAX_ROUTE_ARGS];
    int tag_processed;
    int tag_found;
    int tag_malformed;
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
    return route->tag_processed;
}

CRW_PRIVATE
int CRW_route_tag_found(CRW_Route *route)
{
    return route->tag_found;
}

CRW_PRIVATE
int CRW_route_tag_malformed(CRW_Route *route)
{
    return route->tag_malformed;
}

CRW_PRIVATE
const char *CRW_route_regex_user(CRW_Route *route)
{
    return route->regex_user;
}

CRW_PRIVATE
const char *CRW_route_regex_crane(CRW_Route *route)
{
    return route->regex_crane;
}

CRW_PRIVATE
int CRW_route_regex_dump(CRW_Route *route, const char *msg)
{
    if (!msg) {
        msg = "dump";
    }
    fprintf(stderr, "%s  user = [%s]\n", msg, CRW_route_regex_user(route));
    fprintf(stderr, "%s crane = [%s]\n", msg, CRW_route_regex_crane(route));
    return 0;
}


CRW_PRIVATE
const char *CRW_route_tag_get_by_idx(CRW_Route *route, int idx)
{
    return route->tags[idx];
}

CRW_PRIVATE
int CRW_route_tag_dump(CRW_Route *route, const char *msg)
{
    int j;
    if (!msg) {
        msg = "dump";
    }
    for (j= 0; j < route->tag_processed; j++) {
        fprintf(stderr, "%s tag[%i]= [%s]\n",
                msg, j, route->tags[j]);
    }
    return 0;
}

CRW_PRIVATE
int CRW_route_all_empty_tags(CRW_Route *route)
{
    int j;
    for (j= 0; j < CRW_MAX_ROUTE_ARGS; j++) {
        if (route->tags[j]) {
            fprintf(stderr, "UNEXPECTED tag[%i]= [%s]\n",
                    j, route->tags[j]);
            return 0;
        }
    }
    return 1;
}

CRW_PRIVATE
int CRW_route_set_tags(CRW_Route *route, const char *tags[], int num)
{
    int j;
    for (j = 0; j < num; j++) {
        route->tags[j] = tags[j];
        route->tag_processed++;
        route->tag_found++;
    }
    return 0;
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
int CRW_route_sum_tag_len(CRW_Route *route)
{
    int len = -1;
    if (route) {
        int j;
        len = 0;
        for (j = 0; j < route->tag_processed; j++) {
            len += strlen(route->tags[j]) + 1;
            /* the leading ':' */
        }
    }
    return len;
}


typedef struct crwroutescanner_ CRW_RouteScanner;
struct crwroutescanner_ {
    int tag_found;
    int tag_processed;
    int tag_malformed;

    const char *route_string;

    void *userdata;

    int (*on_tag)(CRW_RouteScanner *RS,
                  int idx, const char *tag, size_t len);
    int (*on_char)(CRW_RouteScanner *RS,
                   int idx, char c);
};

CRW_PRIVATE
int CRW_route_scan_string(CRW_RouteScanner *RS)
{
    /* no nested tags are allowed, so this is quite straightforward */
    int err = 0;
    int outside_tag = 1;
    const char *tag_begin = NULL;
    const char *rt = RS->route_string;
    size_t j = 0, len = strlen(rt);
    for (j = 0; !err && j < len + 1; j++) {
        char c = rt[j];
        if (tag_begin && (c == '\0' || c == '/' || c == ':')) {
            /* now we are at the tag end boundary, so we can process
               the tag we just found */
            size_t tag_len = (&rt[j] - tag_begin);
            if (tag_len >= 1) {
                err = RS->on_tag(RS, j, tag_begin, tag_len);
                RS->tag_processed++;
            } else {
                RS->tag_malformed++;
            }
            tag_begin = NULL;
            outside_tag = 1;
        }
        if (c == ':') {
            /* found a new tag preamble. Record the beginning. */
            if (RS->tag_processed < CRW_MAX_ROUTE_ARGS) {
                tag_begin = &rt[j + 1];
            }
            RS->tag_found++;
            outside_tag = 0;
        }
        if (outside_tag) {
            /* anything else outside a tag. */
            err = RS->on_char(RS, j, c);
        }
    }
    return err;
}

CRW_PRIVATE
int CRW_route_scan_on_char_null(CRW_RouteScanner *RS,
                                int idx, char c)
{
    return 0;
}

CRW_PRIVATE
int CRW_scan_regex_on_tag(CRW_RouteScanner *RS,
                          int idx, const char *tag, size_t len)
{
    CRW_Route *route = RS->userdata;
    route->tags[RS->tag_processed] = tag;
    route->regex_tags[idx] = '\0';
    return 0;
}

CRW_PRIVATE
int CRW_route_scan_regex(CRW_Route *route)
{
    int err = -1;
    if (route) {
        CRW_RouteScanner RS = {
            0, 0, 0,
            route->regex_tags,
            route,
            CRW_scan_regex_on_tag,
            CRW_route_scan_on_char_null
        };
        err = CRW_route_scan_string(&RS);
        if (!err) {
            route->tag_processed = RS.tag_processed;
            route->tag_malformed = RS.tag_malformed;
            route->tag_found     = RS.tag_found;
        }
    }
    return err;
}

typedef struct crwregexbuilder_ CRW_RegexBuilder;
struct crwregexbuilder_ {
    char *ptr;
    int idx;
};

CRW_PRIVATE
int CRW_build_regex_on_char(CRW_RouteScanner *RS,
                            int idx, char c)
{
    CRW_RegexBuilder *RB = RS->userdata;
    RB->ptr[RB->idx] = c;
    RB->idx++;
    return 0;
}

CRW_PRIVATE
int CRW_build_regex_on_tag(CRW_RouteScanner *RS,
                           int idx, const char *tag, size_t len)
{
    CRW_RegexBuilder *RB = RS->userdata;
    strcat(&RB->ptr[RB->idx], SUBEXPR_STR);
    RB->idx += SUBEXPR_LEN;
    return 0;
}


CRW_PRIVATE
int CRW_route_build_crane_regex(CRW_Route *route)
{
    int err = -1;
    if (route) {
        int tags_len = CRW_route_sum_tag_len(route);
        int subx_len = route->tag_processed * SUBEXPR_LEN;
        size_t regx_len = strlen(route->regex_user);
        size_t overhead = 1; /* the ending '\0' */

        if (subx_len > tags_len) {
            overhead += subx_len - tags_len;
        }
        route->regex_crane = calloc(1, regx_len + overhead);
        if (route->regex_crane) {
            CRW_RegexBuilder RB = {
                route->regex_crane,
                0
            };
            CRW_RouteScanner RS = {
                0, 0, 0,
                route->regex_user,
                &RB,
                CRW_build_regex_on_tag,
                CRW_build_regex_on_char
            };
            err = CRW_route_scan_string(&RS);
        }
    }
    return err;
}

CRW_PRIVATE
int CRW_route_init_regex(CRW_Route *route)
{
    int err = 0;
    err = CRW_route_scan_regex(route);
    if (err) {
        CRW_panic("rtr", "error=[%i] while scanning regex for tags", err);
        return err;
    }
    err = CRW_route_build_crane_regex(route);
    if (err) {
        CRW_panic("rtr", "error=[%i] while building the internal regex", err);
        return err;
    }
    err = regcomp(&route->RE, route->regex_crane, REG_EXTENDED);
    if (err) {
        CRW_panic("rtr", "error=[%i] while compiling the internal regex", err);
        return err;
    }
    route->compiled = 1;
    return err;
}

CRW_PRIVATE
int CRW_route_setup(CRW_Route *route, const char *regex)
{
    int err = -1;
    memset(route, 0, sizeof(CRW_Route));
    route->compiled = 0;
    route->regex_user = strdup(regex);
    route->regex_tags = strdup(regex);
    if (route->regex_user && route->regex_tags) {
        err = 0;
    }
    return err;
}

CRW_PRIVATE
int CRW_route_init(CRW_Route *route, const char *regex)
{
    int err = -1;
    if (route && regex) {
        err = CRW_route_setup(route, regex);
        if (!err) {
            err = CRW_route_init_regex(route);
        } else {
            CRW_panic("rtr", "no memory for route data on [%s]", regex);
            err = 1;
        }
        if (err) {
            CRW_route_cleanup(route);
            err = -1;
        }
    }
    return err;
}

CRW_PRIVATE
int CRW_route_match(CRW_Route *route, const char *URI)
{
    int match = 0;
    if (route && URI) {
        int err = regexec(&route->RE, URI,
                          CRW_MAX_ROUTE_ARGS, route->matches,
                          0);
        if (!err) {
            if (route->matches[0].rm_so != -1
             && route->matches[0].rm_eo != -1) {
                match = 1;
            }
        } else {
            /* FIXME */
            CRW_panic("rte", "regexec() failed on URI=[%s] error=(%i)",
                      URI, err);
        }
    }
    return match;
}

CRW_PRIVATE
int CRW_route_fetch(CRW_Route *route, const char *URI,
                    CRW_RouteArgs *args)
{
    int err = -1;
    if (URI && args && route) {
        free(route->data);
        route->data = strdup(URI);
        memset(args, 0, sizeof(*args));
        if (route->data) {
            int j = 0;
            for (j = 0; route->matches[j+1].rm_so != -1
                     && route->matches[j+1].rm_eo != -1; j++) {
                args->pairs[j].key = route->tags[j];
                args->pairs[j].value = &route->data[route->matches[j+1].rm_so];
                route->data[route->matches[j+1].rm_eo] = '\0';
                /* watch out for the bug lurking here */
                args->num++;
                /* FIXME */
            }
            err = 0;
        }
    }
    return err;
}

/*** dispatcher **********************************************************/

typedef struct crwhandlerbinding_ CRW_HandlerBinding;
struct crwhandlerbinding_ {
    CRW_Route route;
    CRW_Handler *handler;
};

struct crwdispatcher_ {
    CRW_Instance *inst;
    list handlers;
};

static CRW_Dispatcher *CRW_dispatcher_new(CRW_Instance *inst)
{
    CRW_Dispatcher *disp = NULL;
    if (inst) {
        disp = calloc(1, sizeof(CRW_Dispatcher));
        if (disp) {
            CRW_dispatcher_init(disp, inst);
        }
    }
    return disp;
}

static void CRW_dispatcher_del(CRW_Dispatcher *disp)
{
    CRW_dispatcher_fini(disp);
    free(disp);
}

static void free_binding(void *data)
{
    CRW_HandlerBinding *HB = data;
    CRW_route_cleanup(&HB->route);
    free(HB);
}

static int CRW_dispatcher_init(CRW_Dispatcher *disp, CRW_Instance *inst)
{
    int err = -1;
    if (disp) {
        disp->inst = inst;
        list_init(&disp->handlers, free_binding);
    }
    return err;
}

static int CRW_dispatcher_fini(CRW_Dispatcher *disp)
{
    int err = -1;
    if (disp) {
        list_destroy(&disp->handlers);
    }
    return err;
}

/* FIXME: found a way to unclutter */
CRW_PRIVATE
int CRW_dispatcher_register(CRW_Dispatcher *disp, const char *route,
                            CRW_Handler *handler)
{
    int err = -1;
    if (disp && route && handler) {
        CRW_HandlerBinding *HB = calloc(1, sizeof(CRW_HandlerBinding));
        if (HB) {
            HB->handler = handler;
            err = CRW_route_init(&HB->route, route);
            if (!err) {
                err = list_insert_next(&disp->handlers, NULL, HB);
                if (!err) {
                    CRW_log(disp->inst, "dsp", CRW_LOG_DEBUG,
                            "bound handler %p for route [%s]",
                            handler, route);
                } else {
                    CRW_log(disp->inst, "dsp", CRW_LOG_ERROR,
                            "failed to bind handler %p for route [%s] error=(%i)",
                            handler, route, err);
                }
            } else {
                CRW_log(disp->inst, "dsp", CRW_LOG_ERROR,
                        "failed intialization for route [%s]",
                        route);
            }
        } else {
            CRW_log(disp->inst, "dsp", CRW_LOG_ERROR,
                    "cannot allocate an handler binding");
        }
    } else {
        CRW_panic("dsp",
                  "invalid parameters for CRW_dispatcher_register");
    }
    return err;
}

CRW_PRIVATE
CRW_Response *CRW_dispatcher_handle(CRW_Dispatcher *disp,
                                    CRW_Request *request)
{
    CRW_Response *res = NULL;
    int found = 0;
    if (disp && request) {
        list_element *elem = NULL;
        CRW_log(disp->inst, "dsp", CRW_LOG_DEBUG,
                "searching handler for URI=[%s]",
                request->URI);
        for (elem = list_head(&disp->handlers);
             !found && elem;
             elem = list_next(elem)) {
            CRW_HandlerBinding *HB = list_data(elem);
            if (CRW_route_match(&HB->route, request->URI)) {
                int err = 0;
                CRW_RouteArgs args;
                found = 1;
                CRW_log(disp->inst, "dsp", CRW_LOG_DEBUG,
                        "handler %p found for URI=[%s] route=[%s]",
                        HB->handler, request->URI, HB->route.regex_user);
                err = CRW_route_fetch(&HB->route, request->URI, &args);
                if (!err) {
                    res = CRW_handler_call(HB->handler, &args, request);
                } else {
                    CRW_log(disp->inst, "dsp", CRW_LOG_ERROR,
                            "route args fetch for URI=[%s] failed error=(%i)",
                            request->URI, err);
                }
            }
        }
    } else {
        CRW_panic("dsp",
                  "invalid parameters for CRW_dispatcher_handle");
    }
    return res;
}




/*** handler *************************************************************/

struct crwhandler_ {
    CRW_Instance *inst;
    const char *route;
    CRW_HandlerCallback callback;
    void *userdata;
};

CRW_Handler *CRW_handler_new(CRW_Instance *inst,
                             const char *route,
                             CRW_HandlerCallback callback,
                             void *userdata)
{
    CRW_Handler *handler = NULL;
    if (inst && inst->disp && route && callback) {
        handler = calloc(1, sizeof(CRW_Handler));
        if (handler) {
            handler->inst = inst;
            handler->route = route;
            handler->callback = callback;
            handler->userdata = userdata;
        }
    }
    return handler;
}

void CRW_handler_del(CRW_Handler *handler)
{
    free(handler);
}

static CRW_Response *CRW_handler_call(CRW_Handler *handler,
                                      const CRW_RouteArgs *args,
                                      const CRW_Request *req)
{
    CRW_Response *res = NULL;
    if (handler && args && req) {
        res = handler->callback(handler->inst,
                                args, req, handler->userdata);
    }
    return res;
}

int CRW_handler_add_route(CRW_Handler *handler, const char *route)
{
    int err = -1;
    if (handler && handler->inst && handler->inst->disp) {
        err = CRW_dispatcher_register(handler->inst->disp, route, handler);
    }
    return err;
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
    const char *options[CRW_MONGOOSE_OPTION_NUM + 1]; /* ending NULL */
    char *hostname;
    char *docroot;
    size_t hostlen;
};

CRW_RequestMethod
CRW_server_adapter_mongoose_method(const struct mg_request_info *request_info)
{
    CRW_RequestMethod meth = CRW_REQUEST_METHOD_UNKNOWN;
    /* FIXME what about a LUT? */
    if (!strcmp(request_info->request_method, "GET")) {
        meth = CRW_REQUEST_METHOD_GET;
    } else if (!strcmp(request_info->request_method, "HEAD")) {
        meth = CRW_REQUEST_METHOD_HEAD;
    } else if (!strcmp(request_info->request_method, "POST")) {
        meth = CRW_REQUEST_METHOD_POST;
    } else if (!strcmp(request_info->request_method, "PUT")) {
        meth = CRW_REQUEST_METHOD_PUT;
    } else if (!strcmp(request_info->request_method, "DELETE")) {
        meth = CRW_REQUEST_METHOD_DELETE;
    } else {
        meth = CRW_REQUEST_METHOD_UNKNOWN;
    }
    return meth;
}

static int CRW_server_adapter_mongoose_build(CRW_ServerAdapter *serv,
                                             const struct mg_request_info *request_info,
                                             CRW_Request *req)
{
    int err = 0;
    if (serv && request_info && req) {
        int j = 0, num = request_info->num_headers;
        if (num > CRW_MAX_REQUEST_HEADERS) {
            num = CRW_MAX_REQUEST_HEADERS;
        }
        req->method = CRW_server_adapter_mongoose_method(request_info);
        req->URI = request_info->uri;
        req->query_string = request_info->query_string;
        for (j = 0; j < num; j++) {
            req->headers[j].key = request_info->http_headers[j].name;
            req->headers[j].value = request_info->http_headers[j].value;
            if (!strcasecmp(req->headers[j].key,
                            "X-Requested-With")) {
                req->is_xhr = 1;
            }
        }
        req->num_headers = num;
    }
    return err;
}


static int CRW_server_adapter_mongoose_send(CRW_ServerAdapter *serv,
                                            struct mg_connection *conn,
                                            CRW_Response *res)
{
    /* FIXME*/
    mg_write(conn, res->body->cstr, res->body->pos);
    return 0;
}


static void *CRW_mongoose_event_handler(enum mg_event event,
                                        struct mg_connection *conn,
                                        const struct mg_request_info *request_info)
{
    void *processed = "craneweb";
    /* always. Mongoose should'nt do anything on its own */
    CRW_ServerAdapter *serv = request_info->user_data;
    CRW_Request *req = CRW_request_new(serv->inst);
    CRW_Response *res = NULL;/*CRW_response_new(serv->inst);*/
    if (req /*&& res*/) {
        int err = 0;
        if (event == MG_NEW_REQUEST) {
            err = CRW_server_adapter_mongoose_build(serv, request_info, req);
            res = CRW_dispatcher_handle(serv->disp, req);
            if (!err && res) {
                err = CRW_server_adapter_mongoose_send(serv, conn, res);
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
    int err = -1;
    if (inst && inst->disp && handler && handler->route) {
        err = CRW_dispatcher_register(inst->disp, handler->route, handler);
    }
    return err;
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

