#ifndef __OPERATIONS_H
#define __OPERATIONS_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../common/error/error.h"
#include "../../common/networking/networking.h"
#include "../../common/networking/requests.h"

#define IS_DIRECTORY 3
#define IS_FILE 4
#define DIR_END_REACH 2

// ErrorCode readFromFile(char* path, char* buffer, size_t bufferSize);
ErrorCode writeToFile(char* path, char* buffer);
ErrorCode createFile(char* path, mode_t permissions);
ErrorCode createDirectory(char* path, mode_t permissions);
ErrorCode deleteFile(char* path);
ErrorCode deleteDirectory(char* path);

ErrorCode CopySend(char* dst);
ErrorCode CopyRecieve(char* src);

ErrorCode GetFileInfo(char* path, struct stat* info);
ErrorCode GetDirectoryInfo(char* path, struct stat* info);
ErrorCode IsDirectory(char* path);
ErrorCode GetDirectorySize(char* path, int* size);

ErrorCode ExecuteWrite(WriteRequest* Req);
ErrorCode ExecuteRead(ReadRequest* Req, int clientfd);
// ErrorCode ExecuteList(ListRequest* Req, int clientfd);
ErrorCode ExecuteMD(MDRequest* Req, int clientfd);
// ErrorCode ExecuteRequest(RequestType reqType, void* request, int clientfd);
// ErrorCode ExecuteClientRequest(RequestType reqType, void* request, int clientfd);
// ErrorCode ExecuteSSRequest(RequestType reqType, void* request, int sockfd);
ErrorCode ExecuteCreate(CreateRequest* Req);
ErrorCode ExecuteDelete(DeleteRequest* Req);
ErrorCode ExecuteCopyRead(char* path, int sockfd);
ErrorCode ExecuteCopyWrite(char* Req, int sockfd);

ErrorCode send_INFOPKT(int sockfd, char* path, bool IsDir);

ErrorCode send_STOPPKT(int sockfd);

ErrorCode send_ENDPKT(int sockfd);

ErrorCode send_DIR_END_PKT(int sockfd);

ErrorCode send_DATAPKT(int sockfd, char* data,int count);

ErrorCode SendfileData(char* filename, int sockfd);

ErrorCode ReceiveFileData(char* filename, int sockfd);
// inside indst path
// ErrorCode getFileSize(char* path, int* size);
// ErrorCode getDirectorySize(char* path, int* size);

// ErrorCode getPermissions(char* path, int* permissions);

#endif