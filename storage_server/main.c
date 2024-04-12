#include <errno.h>

#include "storage_server.h"

int main() {
    if (initSS(&ss)) {
        destroySS(&ss);
        return FAILURE;
    }

    if (connectToNM(&ss)) {
        destroySS(&ss);
        return FAILURE;
    }

    startAliveSocketThread(&ss.aliveThread);

    while (!isCleaningUp()) {
        int sockfd;  // sockfd of requestor
        bool didConnect;
        Requests* requests = &ss.requests;
        if (acceptClientNonBlocking(ss.clientSockfd, &sockfd, &didConnect)) {
            eprintf("Main : Could not accept requestors, errno = %d, %s\n", errno, strerror(errno));
            initiateCleanup(FAILURE);
            break;
        }

        if (didConnect) {
            lprintf("Main : requestor connected\n");
            RequestType type;
            if (recieveRequestType(&type, sockfd)) {
                eprintf("Main : Could not recieve request type\n");
                initiateCleanup(FAILURE);
                goto cleanup;
            }
            lprintf("Main : Recieved request type %d, %s", type, REQ_TYPE_TO_STRING[type]);

            RequestTypeAck reqTypeAck;
            reqTypeAck.requestType = type;
            if (sendRequestTypeAck(&reqTypeAck, sockfd)) {
                eprintf("Could not send request type ack\n");
                initiateCleanup(FAILURE);
                goto cleanup;
            }
            lprintf("Main : Sent RequestTypeAck");
            void* request = allocateRequest(type);
            if (recieveRequest(type, request, sockfd)) {
                free(request);
                eprintf("Main : Could not recieve request\n");
                initiateCleanup(FAILURE);
                goto cleanup;
            }
            lprintf("Main : Recieved request");
            pthread_mutex_lock(&requests->requestLock);
            addRequest(requests, type, request, sockfd);
            pthread_mutex_unlock(&requests->requestLock);
        } else {
            pthread_mutex_lock(&requests->requestLock);
            for (Request itr = requests->front; itr != NULL;) {
                Request next = itr->next;
                // make a function for these 3 lines
                pthread_mutex_lock(&itr->cleanupLock);
                bool isDone = itr->isDone;
                pthread_mutex_unlock(&itr->cleanupLock);
                if (isDone) {
                    removeRequest(requests, itr);
                }
                itr = next;
            }
            pthread_mutex_unlock(&requests->requestLock);
        }
    cleanup:
        // RequestType type;
    }

    destroySS(&ss);
}