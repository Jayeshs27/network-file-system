#ifndef __SS_CONNECT_H
#define __SS_CONNECT_H

#include "../../error/error.h"

#define MAX_PATH_COUNT 256
#define MAX_PATH_LEN 256

typedef struct AccessiblePaths {
    int count;
    char pathList[MAX_PATH_COUNT][MAX_PATH_LEN]; /* List of paths */
} AccessiblePaths;

typedef struct SSInitRequest {
    char cwd[MAX_PATH_LEN];
    int SSPassivePort; /* NM will connect to SS on this port for communication */
    int SSAlivePort;   /* NM will check if SS is alive on this port */
    int SSClientPort;  /* NM will store this port for communcation between SS and client */
    AccessiblePaths paths;
} SSInitRequest;

ErrorCode sendSSRequest(int nmSockfd, SSInitRequest* req);
ErrorCode recieveSSRequest(int ssListenSockfd, SSInitRequest* req);

#endif