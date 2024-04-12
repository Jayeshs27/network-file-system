#ifndef __SSREQ_HANDLER
#define __SSREQ_HANDLER


#include <pthread.h>

#include "../../../common/networking/nm_client/client_connect.h"
#include "../../../common/networking/requests.h"


typedef struct SSrequest {
    pthread_t thread;
    RequestType type;
    void* request;
    SSInfo ssCopyInfo[2];

    pthread_mutex_t cleanupLock;
    bool isCleaningup;
    bool isDone;

    struct SSrequest* next;
    struct SSrequest* prev;
    
}SSrequestStruct;

typedef SSrequestStruct* ssRequest;

void startSSrequestThread(ssRequest request);
void killSSrequestThread(ssRequest request);

typedef struct ssRequests {
    int count;
    ssRequest front;
    ssRequest rear;
    pthread_mutex_t ssRequestsLock;
}ssRequests;

void* SSrequestRoutine(void* arg);

ErrorCode initSSRequests(ssRequests* requests);
void destroySSRequests(ssRequests* requests);

/* Requires requestLock to be held */
ErrorCode addSSRequest(ssRequests* requests, RequestType type, void* request, SSInfo ssCopy1, SSInfo ssCopy2);

/* Requires requestLock to be held */
ErrorCode removeSSRequest(ssRequests* requests, ssRequest request);

bool isCleaningUp();
void initiateCleanup(ErrorCode exitCode);

ErrorCode createSSReqListenerThread(pthread_t* ssReqlistenerThread);
void startSSrequestThread(ssRequest request);

void killSSrequestThread(ssRequest request);
void* ssReqListenerRoutine(void* arg);


#endif