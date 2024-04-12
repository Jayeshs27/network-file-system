#include "create.h"

ErrorCode sendCreateRequest(CreateRequest* request, int sockfd) {
    return __sendRequest(request, sizeof(CreateRequest), sockfd);
}

ErrorCode recieveCreateRequest(CreateRequest* request, int sockfd) {
    return __recieveRequest(request, sizeof(CreateRequest), sockfd);
}