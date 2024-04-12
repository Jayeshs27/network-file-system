#include "client_handler_thread.h"

#include "client_handler.h"

bool isCleaning(ConnectedClient client) {
    pthread_mutex_lock(&client->cleanupLock);
    bool ret = client->isCleaningup;
    pthread_mutex_unlock(&client->cleanupLock);
    return ret;
}

void cleanupClient(ConnectedClient client) {
    lprintf("Client Thread (Alive port = %d) : Cleaning up", client->clientInitRequest.clientAlivePort);
    pthread_mutex_destroy(&client->cleanupLock);
}

void* clientThreadRoutine(void* arg) {
    ConnectedClient client = (ConnectedClient)arg;
    while (!isCleaningUp() && !isCleaning(client)) {
        RequestType reqType;
        RequestTypeAck ack;

        lprintf("Client Thread (Alive port = %d) : Waiting for client", client->clientInitRequest.clientAlivePort);
        if (isCleaning(client)) break;

        if (recieveRequestType(&reqType, client->clientSockfd)) {
            killClientThread(client);
            break;
        }
        lprintf("Client Thread (Alive port = %d) : Recieved %d %s", client->clientInitRequest.clientAlivePort, reqType, REQ_TYPE_TO_STRING[reqType]);

        if (sendRequestTypeAck(&ack, client->clientSockfd)) {
            killClientThread(client);
            break;
        }

        lprintf("Client Thread (Alive port = %d) : Sent RequestTypeAck", client->clientInitRequest.clientAlivePort);

        void* request = allocateRequest(reqType);
        if (recieveRequest(reqType, request, client->clientSockfd)) {
            free(request);
            killClientThread(client);
            break;
        }
        lprintf("Client Thread (Alive port = %d) : Recieved Request", client->clientInitRequest.clientAlivePort);

        /* LIST REQUEST HANDLED DIFFERENT FROM REST */
        if (reqType == REQUEST_LIST) {
            handleListRequest(request, client);
            goto free_request;
        } else if (!isPrivileged(reqType)) {
            handleNP(reqType, request, client);
            goto free_request;
        } else if (reqType == REQUEST_COPY) {
            ErrorCode ret = Copyhandler(request);
            FeedbackAck feedbackAck;
            feedbackAck.errorCode = ret;
            if (sendFeedbackAck(&feedbackAck, client->clientSockfd)) {
                eprintf("Could not send FeedbackAck to client");
                free(request);
                killClientThread(client);
                break;
            }
            if (ret == FAILURE) {
                free(request);
                killClientThread(client);
                break;
            }

        } else if (reqType == REQUEST_CREATE) {
            handleCreateRequest(request, client);
        } else if (reqType == REQUEST_DELETE) {
            handleDeleteRequest(request, client);
        } else {
            char RequestPath[MAX_PATH_SIZE];
            ExtractPath(request, reqType, RequestPath);
            lprintf("Got Path %s", RequestPath);
            SSInfo ssinfo;
            lockTrie();
            ErrorCode ret = search(RequestPath, &ssinfo);
            unlockTrie();
            lprintf("Operation ssclientInfo = %d, sspassiveport = %d", ssinfo.ssClientPort, ssinfo.ssPassivePort);
            // sendSSInfo(&ssinfo, client->clientSockfd);
            if (isPrivileged(reqType)) {
                if (ret == EPATHNOTFOUND) {
                    FeedbackAck feedbackAck;
                    feedbackAck.errorCode = ret;
                    if (sendFeedbackAck(&feedbackAck, client->clientSockfd)) {
                        eprintf("Could not send FeedbackAck to client");
                        free(request);
                        killClientThread(client);
                        break;
                    }

                    lprintf("Client Thread (Alive port = %d) : FeedbackAck sent", client->clientInitRequest.clientAlivePort);
                } else {
                    int sockfd;
                    if (createActiveSocket(&sockfd)) {
                        eprintf("Could not Create Active Socket");
                        free(request);
                        killClientThread(client);
                        break;
                    }

                    lprintf("Client Thread (Alive port = %d) : Active Socket Created", client->clientInitRequest.clientAlivePort);

                    if (connectToServer(sockfd, ssinfo.ssClientPort)) {
                        eprintf("Could not connect to SS");
                        free(request);
                        killClientThread(client);
                        break;
                    }

                    lprintf("Client Thread (Alive port = %d) : Connect to Server(SS)", client->clientInitRequest.clientAlivePort);

                    if (sendRequestType(&reqType, sockfd)) {
                        eprintf("Could not send requestType");
                        free(request);
                        killClientThread(client);
                        break;
                    }

                    lprintf("Client Thread (Alive port = %d) : RequestType sent", client->clientInitRequest.clientAlivePort);

                    if (sendRequest(reqType, request, sockfd)) {
                        eprintf("Could not send Request");
                        free(request);
                        killClientThread(client);
                        break;
                    }

                    lprintf("Client Thread (Alive port = %d) : Request sent", client->clientInitRequest.clientAlivePort);

                    FeedbackAck feedbackAck;
                    if (recieveFeedbackAck(&feedbackAck, sockfd)) {
                        eprintf("Could not recieve FeedbackAck from SS");
                        free(request);
                        killClientThread(client);
                        break;
                    }

                    lprintf("Client Thread (Alive port = %d) : FeedbackAck Recieved", client->clientInitRequest.clientAlivePort);

                    if (sendFeedbackAck(&feedbackAck, client->clientSockfd)) {
                        eprintf("Could not send FeedbackAck to client");
                        free(request);
                        killClientThread(client);
                        break;
                    }

                    lprintf("Client Thread (Alive port = %d) : FeedbackAck sent(to client)", client->clientInitRequest.clientAlivePort);
                    close(sockfd);
                }
            }
        }
    free_request:
        // free(request);
    }
    cleanupClient(client);
    return NULL;
}

void startClientThread(ConnectedClient client) {
    pthread_create(&client->thread, NULL, clientThreadRoutine, (void*)client);
}

void killClientThread(ConnectedClient client) {
    pthread_mutex_lock(&client->cleanupLock);
    client->isCleaningup = true;
    pthread_mutex_unlock(&client->cleanupLock);
}