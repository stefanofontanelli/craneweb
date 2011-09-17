/*
 * craneweb.h -- modern micro web framework for C.
 * (C) 2011 Francesco Romani <fromani at gmail dot com>
 * ZLIB licensed.
 */

#ifndef CRANEWEB_H
#define CRANEWEB_H

#include "config.h"


/*** logger **************************************************************/
/* TODO */

/*** instance ************************************************************/
typedef enum crwserveradptertype_ {
    CRW_SERVER_ADAPTER_DEFAULT = 0,
    CRW_SERVER_ADAPTER_MONGOOSE
} CRW_ServerAdapterType;

typedef struct crwinstance_ CRW_Instance;

CRW_Instance *CRW_instance_new(CRW_ServerAdapterType server);
void CRW_instance_del(CRW_Instance *inst);

/*** request *************************************************************/
typedef struct crwrequest_ CRW_Request;
/* TODO */

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
typedef struct crwhandler_ CRW_Handler;

int CRW_route_args_count(const CRW_RouteArgs *args);
const char *CRW_route_args_get_by_idx(const CRW_RouteArgs *args, int idx);

typedef CRW_Response *(CRW_HandlerCallback)(CRW_Instance *inst,
                                            const CRW_RouteArgs *args,
                                            const CRW_Request *req,
                                            void *userdata);

CRW_Handler *CRW_handler_new(CRW_Instance *inst,
                             const char *route,
                             CRW_HandlerCallback callback,
                             void *userdata);
void CRW_handler_del(CRW_Handler *handler);
CRW_Handler *CRW_handler_add_route(CRW_Handler *handler,
                                   const char *route);

/*** runtime(!) **********************************************************/

typedef struct crwconfig_ CRW_Config;
struct crwconfig_ {
    const char *host;
    int port;
};

int CRW_run(CRW_Instance *instance, const CRW_Config *cfg);

/*** that's all folks! ***************************************************/

#endif /* CRANEWEB_H */
/* EOF!*/

