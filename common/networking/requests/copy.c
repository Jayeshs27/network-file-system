#include "copy.h"

ErrorCode sendCopyRequest(CopyRequest* request, int sockfd) {
    return __sendRequest(request, sizeof(CopyRequest), sockfd);
}

ErrorCode recieveCopyRequest(CopyRequest* request, int sockfd) {
    return __recieveRequest(request, sizeof(CopyRequest), sockfd);
}

ErrorCode sendSSCopyRequest(SSCopyRequest* request, int sockfd) {
    return __sendRequest(request, sizeof(SSCopyRequest), sockfd);
}

ErrorCode recieveSSCopyRequest(SSCopyRequest* request, int sockfd) {
    return __recieveRequest(request, sizeof(SSCopyRequest), sockfd);
}

void CleanPacket(CopyPacket* packet){
    memset(packet->ObjectInfo.ObjectName,'\0',MAX_PATH_SIZE);
    memset(packet->Data,'\0',MAX_DATA_LENGTH);
    packet->count = 0;
}

ErrorCode SendPacket(int sockfd, CopyPacket* packet) {
    if (socketSend(sockfd, packet, sizeof(CopyPacket))) {
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode ReceivePacket(int sockfd, CopyPacket* packet) {
    CleanPacket(packet);
    if (socketRecieve(sockfd, packet, sizeof(CopyPacket))) {
        return FAILURE;
    }
    return SUCCESS;
}