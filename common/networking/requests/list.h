#ifndef __LIST_H
#define __LIST_H

#include <stdbool.h>

#include "../../error/error.h"
#include "../requests.h"

#define MAX_PATH_LENGTH 128
#define MAX_ENTITY_COUNT 128

typedef struct ListRequest {
    char path[MAX_PATH_LENGTH];
} ListRequest;

typedef struct ListResponse {
    int list_cnt;
    char list[MAX_ENTITY_COUNT][MAX_PATH_LENGTH];
} ListResponse;

ErrorCode sendListRequest(ListRequest* request, int sockfd);
ErrorCode recieveListRequest(ListRequest* request, int sockfd);

#endif