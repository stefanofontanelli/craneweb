/*
 * headers.c -- very basic cranweb example.
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

static CRW_Response *headers(CRW_Instance *inst,
                           const CRW_RouteArgs *args,
                           const CRW_Request *req,
                           void *userdata)
{
    CRW_Response *res = CRW_response_new(inst);
    char respbuf[RESP_BUF_SIZE] = { '\0' };
    int j = 0, err = 0, num = CRW_request_count_headers(req);
    CRW_response_add_body(res, "<p>");
    for (j = 0; !err && j < num; j++) {
        const char *name = NULL;
        const char *value = NULL;
        int err = CRW_request_get_header_by_idx(req, j, &name, &value);
        if (!err) {
            snprintf(respbuf, sizeof(respbuf), "%s:%s<br/>", name, value);
            CRW_response_add_body(res, respbuf);
        }
    }
    CRW_response_add_body(res, "</p>");
    return res;
}

int main(int argc, char *argv[])
{
    CRW_Config cfg;
    CRW_Instance *inst = CRW_instance_new(CRW_SERVER_ADAPTER_DEFAULT);
    CRW_Handler *handler = CRW_handler_new(inst, "/headers", headers, NULL);

    CRW_handler_add_route(handler, "/");

    CRW_instance_add_handler(inst, handler);
    
    cfg.host = "127.0.0.1";
    cfg.port = 8080;
    cfg.document_root = "."; /* YMMV? default? */

    return CRW_run(inst, &cfg);
}

