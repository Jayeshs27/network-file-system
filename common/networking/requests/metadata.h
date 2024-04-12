#ifndef __METADATA_H
#define __METADATA_H


#include "../../error/error.h"
#include "../requests.h"

#define MAX_PATH_SIZE 256

typedef struct MDRequest {
    char path[MAX_PATH_SIZE];
} MDRequest;

ErrorCode sendMDRequest(MDRequest* request, int sockfd);
ErrorCode recieveMDRequest(MDRequest* request, int sockfd);

#endif