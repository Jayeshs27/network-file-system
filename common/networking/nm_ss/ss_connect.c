#include "ss_connect.h"

#include "../networking.h"

ErrorCode sendSSRequest(int nmSockfd, SSInitRequest* req) {
    if (socketSend(nmSockfd, req, sizeof(SSInitRequest))) {
        return FAILURE;
    }
    return SUCCESS;
}
ErrorCode recieveSSRequest(int ssListenSockfd, SSInitRequest* req) {
    if (socketRecieve(ssListenSockfd,req, sizeof(SSInitRequest))){
        return FAILURE;
    }
    return SUCCESS;
}