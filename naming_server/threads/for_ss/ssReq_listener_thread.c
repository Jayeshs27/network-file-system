#include "ssReq_handler.h"

#include "../../trie/trie.h"
#include "../../naming_server.h"

ErrorCode createSSReqListenerThread(pthread_t* ssReqlistenerThread) {
    int ret;
    if ((ret = pthread_create(ssReqlistenerThread, NULL, ssReqListenerRoutine, NULL))) {
        eprintf("Could not create SS Request listener thread, errno = %d, %s\n", ret, strerror(ret));
        return FAILURE;
    }
    return SUCCESS;
}

void startSSrequestThread(ssRequest request){
    pthread_create(&request->thread, NULL, SSrequestRoutine, request);
}

void killSSrequestThread(ssRequest request){
    pthread_mutex_lock(&request->cleanupLock);
    request->isCleaningup = true;
    pthread_mutex_unlock(&request->cleanupLock);
}

void* ssReqListenerRoutine(void* arg) {

    while (!isCleaningUp()) {
        lprintf("ssReq_listener : Waiting for ssRequest...");
        int ssSockfd;
        ssRequests* SsRequests = &namingServer.SsRequests;
        if (acceptClient(namingServer.ssReqListenerSockfd, &ssSockfd)) {
            initiateCleanup(SUCCESS);
            break;
        }

        lprintf("SS(Request sender) : connected");
        RequestType type;
        if (recieveRequestType(&type,ssSockfd)) {
           // error
        }
        void* request = allocateRequest(type);
        if (recieveRequest(type,request,ssSockfd)){
           // error
        }
        SSInfo ssCopy1,ssCopy2;
        /// intiliaze sscopy1 and ssCopy2;
        pthread_mutex_lock(&SsRequests->ssRequestsLock);
        addSSRequest(SsRequests, type, request,ssCopy1,ssCopy2);
    
        for (ssRequest itr = SsRequests->front; itr != NULL;) {
            ssRequest next = itr->next;
            // make a function for these 3 lines
            pthread_mutex_lock(&itr->cleanupLock);
            bool isDone = itr->isDone;
            pthread_mutex_unlock(&itr->cleanupLock);
            if (isDone) {
                removeSSRequest(SsRequests, itr);
            }
            itr = next;
        }

        pthread_mutex_unlock(&SsRequests->ssRequestsLock);
    }
    
    close(namingServer.ssReqListenerSockfd);
    return NULL;
}

