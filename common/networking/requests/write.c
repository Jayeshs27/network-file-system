#include "write.h"

ErrorCode sendWriteRequest(WriteRequest* request, int sockfd) {
    return __sendRequest(request, sizeof(WriteRequest), sockfd);
}

ErrorCode recieveWriteRequest(WriteRequest* request, int sockfd) {
    return __recieveRequest(request, sizeof(WriteRequest), sockfd);
}