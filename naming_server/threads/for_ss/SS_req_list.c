#include "ssReq_handler.h"

#include <assert.h>
#include <stdlib.h>

ssRequest newSSRequest(RequestType type, void* request,SSInfo ssCopy1, SSInfo ssCopy2) {
    ssRequest ret = calloc(1, sizeof(SSrequestStruct));
    assert(ret);
    ret->type = type;
    ret->request = request;
    ret->isCleaningup = false;
    ret->isDone = false;
    ret->ssCopyInfo[0] = ssCopy1;
    ret->ssCopyInfo[1] = ssCopy2;
    pthread_mutex_init(&ret->cleanupLock, NULL);
    return ret;
}

void deleteSSRequest(ssRequest request) {
    pthread_mutex_destroy(&request->cleanupLock);
    free(request->request);
    free(request);
}

static void linkSSRequest(ssRequest a, ssRequest b) {
    if (a) a->next = b;
    if (b) b->prev = a;
}

ErrorCode initSSRequests(ssRequests* requests) {
    requests->count = 0;
    requests->front = NULL;
    requests->rear = NULL;
    pthread_mutex_init(&requests->ssRequestsLock, NULL);
    return SUCCESS;
}

void destroySSRequests(ssRequests* requests) {
    pthread_mutex_destroy(&requests->ssRequestsLock);
    for (ssRequest itr = requests->front; itr != NULL;) {
        ssRequest nxt = itr->next;
        deleteSSRequest(itr);
        itr = nxt;
    }
}

ErrorCode addSSRequest(ssRequests* requests, RequestType type, void* request, SSInfo ssCopy1, SSInfo ssCopy2) {
    lprintf("creating RDD thread");
    assert(requests->count >= 0);
    ssRequest node = newSSRequest(type, request, ssCopy1, ssCopy2);
    linkSSRequest(requests->rear, node);
    requests->rear = node;
    requests->count++;
    if (requests->front == NULL) requests->front = requests->rear;
    startSSrequestThread(node);
    return SUCCESS;
}

ErrorCode removeSSRequest(ssRequests* requests, ssRequest request) {
    assert(requests->count > 0);
    assert(request);
    requests->count--;
    ssRequest front = requests->front;
    ssRequest rear = requests->rear;
  
    if (request == front) requests->front = front->next;
    if (request == rear) requests->rear = rear->prev;
    linkSSRequest(request->prev, request->next);
    killSSrequestThread(request);
    // printf("killed!\n");
    pthread_join(request->thread, NULL);
    return SUCCESS;
}