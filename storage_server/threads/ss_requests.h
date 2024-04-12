#ifndef __SS_REQUESTS_H
#define __SS_REQUESTS_H

#include <pthread.h>
#include <stdbool.h>

#include "../../common/error/error.h"
#include "../../common/networking/requests.h"
#include "../operations/operations.h"

typedef struct RequestStruct {
    pthread_t thread;
    RequestType type;
    void* request;
    int sockfd;

    /* Cleanup stuff */
    pthread_mutex_t cleanupLock;
    bool isCleaningup;
    bool isDone;

    struct RequestStruct* next;
    struct RequestStruct* prev;
} RequestStruct;

typedef RequestStruct* Request;

void startRequestThread(Request request);
void killRequestThread(Request request);

typedef struct Requests {
    int count;
    Request front;
    Request rear;
    pthread_mutex_t requestLock;
} Requests;

ErrorCode initRequests(Requests* requests);
void destroyRequests(Requests* requests);

/* Requires requestLock to be held */
ErrorCode addRequest(Requests* requests, RequestType type, void* request, int sockfd);

/* Requires requestLock to be held */
ErrorCode removeRequest(Requests* requests, Request request);

bool isCleaningUp();
void initiateCleanup(ErrorCode exitCode);

#endif