#include <signal.h>

#include "../../common/print/logging.h"
#include "ss_requests.h"

bool isCleaning(Request request) {
    pthread_mutex_lock(&request->cleanupLock);
    bool ret = request->isCleaningup;
    pthread_mutex_unlock(&request->cleanupLock);
    return ret;
}
// bool isCopyOfss(void* request,RequestType type){
//     switch(type){
//     case REQUEST_CREATE:
//         CreateRequest* Creq = (CreateRequest*)request;
//         return Creq->IsCopy;
//     case REQUEST_DELETE:
//         DeleteRequest* Rreq = (DeleteRequest*)request;
//         return Rreq->IsCopy;
//     case REQUEST_WRITE:
//         WriteRequest* Wreq = (WriteRequest*)request;
//         return Wreq->IsCopy;
//     case REQUEST_COPY_WRITE:
//         SSCopyRequest* Sreq = (SSCopyRequest*)request;
//         return Sreq->IsCopy;
//     default:
//     }
//     return false;
// }

// ErrorCode ModifySScopy(void* request,RequestType type){
//     int sockfd;
//     if(createActiveSocket(&sockfd)){
//         // 
//     }
//     if(connectToServer(sockfd, SS_REQ_LISTEN_PORT)){
//         // 
//     }
//     if(sendRequestType(&type,sockfd)){
//         //
//     }
//     if(sendRequest(type,request,sockfd)){

//     }
//     close(sockfd);
//     return SUCCESS;
// }
void* requestRoutine(void* arg) {
    Request requestThread = (Request)arg;
    RequestType type = requestThread->type;
    void* request = requestThread->request;
    int sockfd = requestThread->sockfd;
    signal(SIGPIPE, SIG_IGN);
    if (!isCleaningUp() && !isCleaning(requestThread)) {
        lprintf("Processing %s", REQ_TYPE_TO_STRING[type]);
        FeedbackAck Ack;
        switch (type) {
            case REQUEST_WRITE:
                Ack.errorCode = ExecuteWrite((WriteRequest*)request);
                break;
            case REQUEST_READ:
                Ack.errorCode = ExecuteRead((ReadRequest*)request, sockfd);
                break;
            case REQUEST_METADATA:
                Ack.errorCode = ExecuteMD((MDRequest*)request, sockfd);
                break;
            case REQUEST_CREATE:
                Ack.errorCode = ExecuteCreate((CreateRequest*)request);
                break;
            case REQUEST_DELETE:
                Ack.errorCode = ExecuteDelete((DeleteRequest*)request);
                break;
            case REQUEST_COPY_READ:
                Ack.errorCode = ExecuteCopyRead(((SSCopyRequest*)request)->path, sockfd);
                Ack.errorCode = send_ENDPKT(sockfd);
                break;
            case REQUEST_COPY_WRITE:
                Ack.errorCode = ExecuteCopyWrite(((SSCopyRequest*)request)->path, sockfd);
                break;
            default:
        }

        lprintf("Sending feedback ack %d", Ack.errorCode);
        if (sendFeedbackAck(&Ack, sockfd)) {
            eprintf("Could not Send FeedbackAck\n");
        }
        requestThread->isDone = true;
        // lprintf("FeedbackAck sent for %s : %d",REQ_TYPE_TO_STRING[type],Ack.errorCode);
        // if(Ack.errorCode == SUCCESS && isModified(type) && isCopyOfss(request,type)){
        //     ModifySScopy(request,type);
        // }
    }
    return NULL;
}

void startRequestThread(Request request) {
    pthread_create(&request->thread, NULL, requestRoutine, request);
}

void killRequestThread(Request request) {
    pthread_mutex_lock(&request->cleanupLock);
    request->isCleaningup = true;
    pthread_mutex_unlock(&request->cleanupLock);
}