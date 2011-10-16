/*
 * craneweb.h -- modern micro web framework for C.
 * (C) 2011 Francesco Romani <fromani at gmail dot com>
 * ZLIB licensed.
 */

/** \file craneweb.h
    \brief The craneweb public interface.
*/

#ifndef CRANEWEB_H
#define CRANEWEB_H

#include "config.h"

#include <stdarg.h>


/*** implementation limits ***********************************************/

/** \enum the craneweb implementation limits.

    It is generally feasible to ramp up those limits but you must be
    prepared and carefully test your build. While craneweb should scale
    gently (and not blowup), the performance and memory cost could be
    not linear.
    TL;DR: tune at your own risk.
*/
enum {
    CRW_MAX_ROUTE_ARGS = 16,      /**< max number of :tags per route */
    CRW_MAX_HANDLER_ROUTES = 16,  /**< max number of aliases per handler */
    CRW_MAX_REQUEST_HEADERS = 64  /**< max number of HTTP headers exported
                                       into a CRW_Request. The remainder
                                       is silently dropped.
                                   */
};
// TODO: doxygen anchor

/*** logger **************************************************************/

/** \enum CRW_LogLevel
    \brief the log message severity levels
*/
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

/** \fn CRW_logger_console
    \brief default logging callback, emitting messages to the console.

    This is the default cranweb logging function. It logs messages
    on the console (stderr).

    \see CRW_LogHandler
*/
int CRW_logger_console(void *userdata, CRW_LogLevel level, const char *tag,
                       const char *fmt, va_list args);

/*** instance (1) ********************************************************/

/** \enum CRW_ServerAdapterType
    \brief the embedded server supported by craneweb
*/
typedef enum crwserveradptertype_ {
    CRW_SERVER_ADAPTER_NONE = -1,   /**< none (you should'nt see this) */
    CRW_SERVER_ADAPTER_DEFAULT = 0, /**< implementation default */
    CRW_SERVER_ADAPTER_MONGOOSE     /**< mongoose */
} CRW_ServerAdapterType;

/** \var typedef CRW_Instance
    \brief A CRW_Instance represent a craneweb application.

    You'll need to instantiate one and just one, and this as to live as long
    as you want to use the cranweb.
    A CRW_Instance has to be fully opaque to the caller.
*/
typedef struct crwinstance_ CRW_Instance;

/*** request *************************************************************/

/** \var typedef CRW_Request
    \brief A CRW_Request abstracts an HTTP request to your application.

    Accessing the fields a CRW_Request allow the application to understand
    what to do and how properly respond to the request itself.

    Both high-level and low-level (raw HTTP fileds) accessors are provided.

    The CRW_Request are provided, already filled and set up, by the 
    craneweb runtime to your application. There is (should) no need
    to build or to free or to modify them on the application.
    Thereof, all the accessor function are getters only.
*/ 
typedef struct crwrequest_ CRW_Request;

/** \enum CRW_RequestMethod
    \brief the HTTP method of the request.
*/
typedef enum crwrequestmethod_ {
    CRW_REQUEST_METHOD_UNSUPPORTED = -1, /**< invalid data marker */
    CRW_REQUEST_METHOD_UNKNOWN = 0,      /**< on error (runtime) */
    CRW_REQUEST_METHOD_GET,              /**< GET */
    CRW_REQUEST_METHOD_HEAD,             /**< HEAD */
    CRW_REQUEST_METHOD_POST,             /**< POST */
    CRW_REQUEST_METHOD_PUT,              /**< PUT */
    CRW_REQUEST_METHOD_DELETE            /**< DELETE */
} CRW_RequestMethod;

/** \fn CRW_request_get_method
    \brief get the HTTP method of a given request

    \param req CRW_Request to be examined.
    \return the CRW_RequestMethod of the given request.

    \see CRW_RequestMethod
*/
CRW_RequestMethod CRW_request_get_method(const CRW_Request *req);

/** \fn CRW_request_count_headers
    \brief tell how many attached HTTP headers are avalaible for
           the given request.

    In the common case, this function returns the number of H
    headers attached to the HTTP request wrapped to craneweb.
    So, the real number of headers passed on the wire.

    However, given the implementation limits of craneweb, on
    certain (rare) cases, only the first N can be provided.
    the remainder of the headers are silently discarded.

    \param req CRW_Request to be examined.
    \return <0 on error, the number of avalaible headers on success.
*/
int CRW_request_count_headers(const CRW_Request *req);

/** \fn CRW_request_get_header_by_idx
    \brief access the header data.

    Access the HTTP header data (in a readonly way) using an index.
    The HTTP headers are indexed from 0 (zero) to N, [0,N) where
    N is the upper bound is provided by CRW_request_count_headers.

    The header data is returned preserving the case.

    \param req CRW_Request to be examined.
    \param idx index of the header data requested.
    \param[out] header const pointer to be set to the idx-th header key.
    \param[out] value const pointer to be set to the idx-th header value.
    \return 0 on success, <0 on error.

    \see CRW_request_count_headers
*/
int CRW_request_get_header_by_idx(const CRW_Request *req, int idx,
                                  const char **header, const char **value);

/** \fn CRW_request_get_header_value
    \brief access the header value given its name.

    Access the HTTP header data (in a readonly way) using the header name.
    The name search is done in a case INsenstive way.
    The header data is returned preserving the case.

    \param req CRW_Request to be examined.
    \param header the header name (key) to be searched.
    \return NULL on error, the value of the header key on success.

    \see CRW_request_get_header_by_idx
    \see CRW_request_count_headers
*/
const char *CRW_request_get_header_value(const CRW_Request *req, const char *header);

/** \fn CRW_request_is_xhr
    \brief checks if the request is made by the client using
           XmlHttpRequest.

    CAUTION: proper detection required that the client code honores the
    custom http header. This call can give wrong result if the client
    is non-compliant.

    \param req CRW_Request to be examined.
    \return !0 if the request is detected made using XmlHttpRequest,
            0 otherwise.
*/
int CRW_request_is_xhr(const CRW_Request *req);

/*** response ************************************************************/


typedef struct crwresponse_ CRW_Response;

CRW_Response *CRW_response_new(CRW_Instance *inst);
void CRW_response_del(CRW_Response *res);

int CRW_response_add_header(CRW_Response *res,
                            const char *name, const char *value);
int CRW_response_add_body(CRW_Response *res, const char *chunk);


/*** route ***************************************************************/

typedef struct crwrouteargs_ CRW_RouteArgs;

int CRW_route_args_count(const CRW_RouteArgs *args);
const char *CRW_route_args_get_by_idx(const CRW_RouteArgs *args, int idx);
const char *CRW_route_args_get_by_tag(const CRW_RouteArgs *args,
                                      const char *tag);

/*** handler *************************************************************/

typedef struct crwhandler_ CRW_Handler;

typedef CRW_Response *(*CRW_HandlerCallback)(CRW_Instance *inst,
                                             const CRW_RouteArgs *args,
                                             const CRW_Request *req,
                                             void *userdata);

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

/** \struct CRW_Config
    \brief runtime configuration of a CRW_Instance.

    Export the tunables of the CRW_Instance.

    Client code needs to properly fill it for running a CRW_Instance.
*/
typedef struct crwconfig_ CRW_Config;
struct crwconfig_ {
    const char *host;           /**< host IP to listen on */
    int port;                   /**< listening port */
    const char *document_root;  /**< document root (static files)*/
};

/**< \fn CRW_run
     \brief runs a CRW_Instance, allowing it to serve requests.

     \param instance the CRW_Instance to be run.
     \param cfg CRW_Config describing how to run the given instance.
     \return 0 on success, <0 on error.

     \see CRW_Instance
     \see CRW_Config
*/
int CRW_run(CRW_Instance *instance, const CRW_Config *cfg);

/*** that's all folks! ***************************************************/

#endif /* CRANEWEB_H */

/* vim: set ts=4 sw=4 et */
/* EOF!*/

