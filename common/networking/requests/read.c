#include "read.h"

ErrorCode sendReadRequest(ReadRequest* request, int sockfd) {
    return __sendRequest(request, sizeof(ReadRequest), sockfd);
}

ErrorCode recieveReadRequest(ReadRequest* request, int sockfd) {
    return __recieveRequest(request, sizeof(ReadRequest), sockfd);
}

ErrorCode send_STOP_PKT(int sockfd) {
    ReadPacket packet;
    packet.header = STOP_PKT;
    if (socketSend(sockfd, &packet, sizeof(ReadPacket))) {
        eprintf("Could not send Stop_packet");
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode sendDataPacket(char* buffer, int sockfd) {
    ReadPacket packet;
    packet.header = DATA_PKT;
    memset(packet.data, '\0', MAX_DATA_LENGTH);
    // strcpy(packet.data,buffer);
    memcpy(packet.data, buffer, MAX_DATA_LENGTH);
    if (socketSend(sockfd, &packet, sizeof(ReadPacket))) {
        eprintf("Could not send Stop_packet");
        return FAILURE;
    }
    return SUCCESS;
}

ErrorCode receivePacket(ReadPacket* packet, int sockfd) {
    if (socketRecieve(sockfd, packet, sizeof(ReadPacket))) {
        eprintf("Could not receive Packet");
        return FAILURE;
    }
    return SUCCESS;
}