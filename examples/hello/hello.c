/*
 * hello.c -- very basic cranweb example.
 * (C) 2011 Francesco Romani <fromani at gmail dot com>
 * ZLIB licensed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "craneweb.h"

enum {
    RESP_BUF_SIZE = 256
};

static CRW_Response hello(CRW_Instance *inst,
                          const CRW_RouteArgs *args,
                          const CRW_Request *req,
                          void *userdata)
{
    CRW_Response *res = CRW_response_new(inst);
    char respbuf[RESP_BUF_SIZE] = { '\0' };
    snprintf(respbuf, sizeof(respbuf),
             "<b>Hello %s!</b>", CRW_route_args_get_by_idx(args, 0));
    CRW_response_add_body(res, respbuf);
    return res;
}

int main(int argc, char *argv[])
{
    CRW_Config cfg;
    CRW_Instance *inst = CRW_instance_new(CRW_SERVER_ADAPTER_DEFAULT);
    CRW_handler_new(inst, "/hello/:name", hello, NULL);
    
    cfg.host = "localhost";
    cfg.port = 8080;

    return CRW_run(inst, &cfg);
}

