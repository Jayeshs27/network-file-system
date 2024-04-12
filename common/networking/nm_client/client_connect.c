#include "client_connect.h"

#include "../networking.h"

ErrorCode sendClientRequest(int nmSockfd, ClientInitRequest* req){
    if (socketSend(nmSockfd, req, sizeof(ClientInitRequest))) {
        return FAILURE;
    }
    return SUCCESS;
}
ErrorCode recieveClientRequest(int ssListenSockfd,ClientInitRequest* req){
    if (socketRecieve(ssListenSockfd,req, sizeof(ClientInitRequest))){
        return FAILURE;
    }
    return SUCCESS;
}