#include "ss_requests.h"

#include <assert.h>
#include <stdlib.h>

Request newRequest(RequestType type, void* request, int sockfd) {
    Request ret = calloc(1, sizeof(RequestStruct));
    assert(ret);
    ret->type = type;
    ret->request = request;
    ret->isCleaningup = false;
    ret->isDone = false;
    ret->sockfd = sockfd;
    pthread_mutex_init(&ret->cleanupLock, NULL);
    return ret;
}

void deleteRequest(Request request) {
    pthread_mutex_destroy(&request->cleanupLock);
    free(request->request);
    free(request);
}

static void linkRequest(Request a, Request b) {
    if (a) a->next = b;
    if (b) b->prev = a;
}

ErrorCode initRequests(Requests* requests) {
    requests->count = 0;
    requests->front = NULL;
    requests->rear = NULL;
    pthread_mutex_init(&requests->requestLock, NULL);
    return SUCCESS;
}

void destroyRequests(Requests* requests) {
    pthread_mutex_destroy(&requests->requestLock);
    for (Request itr = requests->front; itr != NULL;) {
        Request nxt = itr->next;
        deleteRequest(itr);
        itr = nxt;
    }
}

ErrorCode addRequest(Requests* requests, RequestType type, void* request, int sockfd) {
    assert(requests->count >= 0);
    Request node = newRequest(type, request, sockfd);
    linkRequest(requests->rear, node);
    requests->rear = node;
    requests->count++;
    if (requests->front == NULL) requests->front = requests->rear;
    startRequestThread(node);
    return SUCCESS;
}

ErrorCode removeRequest(Requests* requests, Request request) {
    assert(requests->count > 0);
    assert(request);
    requests->count--;
    Request front = requests->front;
    Request rear = requests->rear;
  
    if (request == front) requests->front = front->next;
    if (request == rear) requests->rear = rear->prev;
    linkRequest(request->prev, request->next);
    killRequestThread(request);
    // printf("killed!\n");
    pthread_join(request->thread, NULL);
    return SUCCESS;
}