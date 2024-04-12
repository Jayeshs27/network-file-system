#ifndef __CREATE_H
#define __CREATE_H

#include <stdbool.h>

#include "../../error/error.h"
#include "../requests.h"

#define MAX_PATH_SIZE 256

typedef struct CreateRequest {
    char path[MAX_PATH_SIZE];
    char newObject[MAX_PATH_SIZE];
    bool isDirectory;
    bool IsCopy;
} CreateRequest;

ErrorCode sendCreateRequest(CreateRequest* request, int sockfd);
ErrorCode recieveCreateRequest(CreateRequest* request, int sockfd);

#endif