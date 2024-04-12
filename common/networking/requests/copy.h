#ifndef __COPY_H
#define __COPY_H

#include "../../error/error.h"
#include "../requests.h"

#define MAX_PATH_SIZE 256
#define MAX_COPY_HEADER 64
#define MAX_DATA_LENGTH 1024  // FOR ONE PACKET

typedef struct CopyRequest {
    char SrcPath[MAX_PATH_SIZE];
    char DestPath[MAX_PATH_SIZE];
    // more content to be defined;
} CopyRequest;

typedef struct SSCopyRequest {
    char path[MAX_PATH_SIZE];
    char otherpath[MAX_PATH_SIZE];
    RequestType reqType;
    bool IsCopy;
} SSCopyRequest;

typedef enum Copyheader {
    STOP, // filestop
    INFO,
    DIR_END, // dir stop
    END,  // last packet
    DATA
} Copyheader;

typedef struct FileInfo {
    char ObjectName[MAX_PATH_SIZE];
    char FullName[MAX_PATH_SIZE];
    bool IsDirectory;
} FileInfo;

typedef struct CopyPacket {
    Copyheader header;
    FileInfo ObjectInfo;
    char Data[1024];
    int count;
} CopyPacket;

ErrorCode sendCopyRequest(CopyRequest* request, int sockfd);
ErrorCode recieveCopyRequest(CopyRequest* request, int sockfd);

ErrorCode sendSSCopyRequest(SSCopyRequest* request, int sockfd);
ErrorCode recieveSSCopyRequest(SSCopyRequest* request, int sockfd);

void CleanPacket(CopyPacket* packet);
ErrorCode SendPacket(int sockfd, CopyPacket* packet);
ErrorCode ReceivePacket(int sockfd, CopyPacket* packet);

#endif
