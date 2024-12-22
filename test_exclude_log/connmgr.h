#ifndef CONNMGR_H
#define CONNMGR_H

#include <pthread.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"

typedef struct {
    int port;
    int max_conn;
} conn_thread_args_t;

void* client_handle(void* arg);
int connmgr_run(int port, int max_conn);

#endif // CONNMGR_H

