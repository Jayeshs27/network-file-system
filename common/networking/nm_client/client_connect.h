#ifndef __CLIENT_CONNECT_H
#define __CLIENT_CONNECT_H

#include "../../error/error.h"

typedef struct ClientInitRequest {
    int clientPassivePort;
    int clientAlivePort;
} ClientInitRequest;

ErrorCode sendClientRequest(int nmSockfd, ClientInitRequest* req);
ErrorCode recieveClientRequest(int ssListenSockfd,ClientInitRequest* req);

#endif