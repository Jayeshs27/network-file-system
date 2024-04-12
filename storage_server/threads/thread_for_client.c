#include "thread_for_client.h"

ErrorCode initThreadForClient(ThreadForClient* thread) {
    lprintf("Main : Creating Passive Socket for SS's Client Socket");
    if (createPassiveSocket(&thread->clientSockfd, 0)) {
        return FAILURE;
    } else {
        lprintf("Main : Getting port for SS's Client Socket");
        if (getPort(thread->clientSockfd, &thread->clientSockPort))
            return FAILURE;
    }
    
    thread->thread = 0;
    return SUCCESS;
}


void* SSClientThreadRoutine(void* arg) {
    ThreadForClient* thread = (ThreadForClient*)arg;
    while (1) {
        int clientfd;
        lprintf("ClientThread : Accepting client...");
        if (acceptClient(thread->clientSockfd, &clientfd)) {
            eprintf("Could not acceptClient\n");
            break;
        }
        lprintf("ClientThread : Client connect");
        RequestType reqType;
        if (recieveRequestType(&reqType, clientfd)) {
            eprintf("Could not Receive RequestType\n");
            break;
        }
        lprintf("ClientThread : requestType Received '%s'", REQ_TYPE_TO_STRING[reqType]);
        RequestTypeAck reqAck;
        if (sendRequestTypeAck(&reqAck, clientfd)) {
            eprintf("Could not send RequesttypeAck\n");
            break;
        }
        lprintf("ClientThread : requestType ack sent");
        void* request = allocateRequest(reqType);
        if (recieveRequest(reqType, request, clientfd)) {
            eprintf("Could not Receive Request\n");
            free(request);
            break;
        }
        lprintf("ClientThread : Request received");
        if (!isPrivileged(reqType)) {
            FeedbackAck Ack;
            lprintf("ClientThread : executing operation");
            Ack.errorCode = ExecuteClientRequest(reqType, request, clientfd);
            lprintf("ClientThread : feedbackAck sent");
            if (sendFeedbackAck(&Ack, clientfd)) {
                eprintf("Could not Send FeedbackAck\n");
                break;
            }
        }
        close(clientfd);
    }
    return NULL;
}
void startThreadForClient(ThreadForClient* thread) {
    pthread_create(&thread->thread, NULL, SSClientThreadRoutine, thread);
}
