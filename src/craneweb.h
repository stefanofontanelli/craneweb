/*
 * craneweb.h -- modern micro web framework for C.
 * (C) 2011 Francesco Romani <fromani at gmail dot com>
 * ZLIB licensed.
 */

#ifndef CRANEWEB_H
#define CRANEWEB_H

#include "config.h"

#include <stdarg.h>


/*** implementation limits ***********************************************/
enum {
    CRW_MAX_ROUTE_ARGS = 16,
    CRW_MAX_HANDLER_ROUTES = 16
};

/*** logger **************************************************************/
/** \enum the message levels */
typedef enum {
    CRW_LOG_CRITICAL = 0, /**< this MUST be the first and it is
                               the most important. -- PANIC!       */
    CRW_LOG_ERROR,        /**< you'll need to see this             */
    CRW_LOG_WARNING,      /**< you'd better to see this            */
    CRW_LOG_INFO,         /**< informative messages (for tuning)   */
    CRW_LOG_DEBUG,        /**< debug messages (for devs)           */
} CRW_LogLevel;


/** \var typedef CRW_LogHandler
    \brief logging callback function.

    This callback is invoked by the logkit runtime whenever is needed
    to log a message.

    \param userdata a pointer given at the callback registration
           time. Fully opaque for craneweb.
    \param tag string identifying the craneweb module/subsystem.
    \param level the severity of the message.
    \param fmt printf-like format string for the message.
    \param args va_list of the arguments to complete the format string.
    \return 0 on success, <0 otherwise.

    \see CRW_LogLevel
*/
typedef int (*CRW_LogHandler)(void *userdata,
                              CRW_LogLevel level, const char *tag,
                              const char *fmt, va_list args);

int CRW_logger_console(void *userdata, CRW_LogLevel level, const char *tag,
                       const char *fmt, va_list args);

/*** instance (1) ********************************************************/
typedef enum crwserveradptertype_ {
    CRW_SERVER_ADAPTER_NONE = -1,
    CRW_SERVER_ADAPTER_DEFAULT = 0,
    CRW_SERVER_ADAPTER_MONGOOSE
} CRW_ServerAdapterType;

typedef struct crwinstance_ CRW_Instance;

/*** request *************************************************************/
typedef struct crwrequest_ CRW_Request;
/* TODO */

CRW_Request *CRW_request_new(CRW_Instance *inst);
void CRW_request_del(CRW_Request *res);

/*** response ************************************************************/
typedef struct crwresponse_ CRW_Response;

CRW_Response *CRW_response_new(CRW_Instance *inst);
void CRW_response_del(CRW_Response *res);
int CRW_response_add_header(CRW_Response *res,
                            const char *name, const char *value);
int CRW_response_add_body(CRW_Response *res, const char *chunk);
int CRW_response_send(CRW_Response *res);


/*** route ***************************************************************/

typedef struct crwrouteargs_ CRW_RouteArgs;

int CRW_route_args_count(const CRW_RouteArgs *args);
const char *CRW_route_args_get_by_idx(const CRW_RouteArgs *args, int idx);
const char *CRW_route_args_get_by_tag(const CRW_RouteArgs *args,
                                      const char *tag);

typedef CRW_Response *(*CRW_HandlerCallback)(CRW_Instance *inst,
                                             const CRW_RouteArgs *args,
                                             const CRW_Request *req,
                                             void *userdata);

/*** handler *************************************************************/

typedef struct crwhandler_ CRW_Handler;

CRW_Handler *CRW_handler_new(CRW_Instance *inst,
                             const char *route,
                             CRW_HandlerCallback callback,
                             void *userdata);
void CRW_handler_del(CRW_Handler *handler);
int CRW_handler_add_route(CRW_Handler *handler, const char *route);


/*** instance (2) ********************************************************/

CRW_Instance *CRW_instance_new(CRW_ServerAdapterType server);
void CRW_instance_del(CRW_Instance *inst);

int CRW_instance_set_logger(CRW_Instance *inst, CRW_LogHandler logger);

int CRW_instance_add_handler(CRW_Instance *inst, CRW_Handler *handler);

/*** runtime(!) **********************************************************/

typedef struct crwconfig_ CRW_Config;
struct crwconfig_ {
    const char *host;
    int port;
    const char *document_root;
};

int CRW_run(CRW_Instance *instance, const CRW_Config *cfg);

/*** that's all folks! ***************************************************/

#endif /* CRANEWEB_H */

/* vim: set ts=4 sw=4 et */
/* EOF!*/

