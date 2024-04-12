#include "list.h"

ErrorCode sendListRequest(ListRequest* request, int sockfd) {
    return __sendRequest(request, sizeof(ListRequest), sockfd);
}

ErrorCode recieveListRequest(ListRequest* request, int sockfd) {
    return __recieveRequest(request, sizeof(ListRequest), sockfd);
}