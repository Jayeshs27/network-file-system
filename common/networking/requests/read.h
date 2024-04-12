#ifndef __READ_H
#define __READ_H

#include "../../error/error.h"
#include "../requests.h"

#define MAX_PATH_SIZE 256
#define MAX_DATA_LENGTH 1024

typedef struct ReadRequest {
    char path[MAX_PATH_SIZE];
}ReadRequest;

typedef enum Readheader{
    STOP_PKT,
    DATA_PKT
}Readheader;

typedef struct ReadPacket {
    Readheader header;
    char data[MAX_DATA_LENGTH];
}ReadPacket;

ErrorCode sendReadRequest(ReadRequest* request, int sockfd);
ErrorCode recieveReadRequest(ReadRequest* request, int sockfd);
ErrorCode send_STOP_PKT(int sockfd);
ErrorCode sendDataPacket(char* buffer,int sockfd);
ErrorCode receivePacket(ReadPacket* packet,int sockfd);

#endif