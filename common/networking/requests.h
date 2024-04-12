#ifndef __NM_CLIENT_REQUESTS_H
#define __NM_CLIENT_REQUESTS_H

#include <stdlib.h>

#include "../error/error.h"
#include "ack/feedback_ack.h"
#include "ack/request_type_ack.h"
#include "ack/ssinfo.h"
#include "networking.h"
#include "requests/copy.h"
#include "requests/create.h"
#include "requests/delete.h"
#include "requests/list.h"
#include "requests/metadata.h"
#include "requests/read.h"
#include "requests/request_type.h"
#include "requests/write.h"

ErrorCode sendRequestType(RequestType* requestType, int sockfd);

ErrorCode recieveRequestType(RequestType* requestType, int sockfd);

ErrorCode sendRequestTypeAck(RequestTypeAck* ack, int sockfd);

ErrorCode recieveRequestTypeAck(RequestTypeAck* ack, int sockfd, int millis, bool* recieved);

ErrorCode __sendRequest(void* request, size_t requestSize, int sockfd);

ErrorCode sendRequest(RequestType requestType, void* request, int sockfd);

ErrorCode __recieveRequest(void* request, size_t requestSize, int sockfd);

ErrorCode recieveRequest(RequestType requestType, void* request, int sockfd);

void* allocateRequest(RequestType requestType);

ErrorCode sendFeedbackAck(FeedbackAck* ack, int sockfd);

ErrorCode recieveFeedbackAck(FeedbackAck* ack, int sockfd);

ErrorCode sendSSInfo(SSInfo* ssinfo, int sockfd);

ErrorCode recieveSSInfo(SSInfo* ssinfo, int sockfd);

#endif