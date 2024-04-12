#include "delete.h"

ErrorCode sendDeleteRequest(DeleteRequest* request, int sockfd) {
    return __sendRequest(request, sizeof(DeleteRequest), sockfd);
}

ErrorCode recieveDeleteRequest(DeleteRequest* request, int sockfd) {
    return __recieveRequest(request, sizeof(DeleteRequest), sockfd);
}