#ifndef __CLIENT_REQUEST_H
#define __CLIENT_REQUEST_H

#include <sys/stat.h>

#include "../common/error/error.h"
#include "../common/networking/requests.h"
#include "../common/print/logging.h"
#include "client_requests.h"

void printFileInfo(struct stat *fileStat);
ErrorCode ReadResponseHandler(int sockfd);
ErrorCode MetaDataResponseHandler(int sockfd);
ErrorCode ListResponseHandler(int sockfd);
ErrorCode ReadResponseHandler(int sockfd);

#endif