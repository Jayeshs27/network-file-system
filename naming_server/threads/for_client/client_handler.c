#include "client_handler.h"

void ExtractPath(void* request, RequestType reqType, char* Path) {
    memset(Path, '\0', MAX_PATH_LEN);
    switch (reqType) {
        case REQUEST_READ:
            ReadRequest* readReq = (ReadRequest*)request;
            memcpy(Path, readReq->path, MAX_PATH_LEN);
            break;
        case REQUEST_WRITE:
            WriteRequest* writeReq = (WriteRequest*)request;
            memcpy(Path, writeReq->path, MAX_PATH_LEN);
            break;
        case REQUEST_METADATA:
            MDRequest* mdReq = (MDRequest*)request;
            memcpy(Path, mdReq->path, MAX_PATH_LEN);
            break;
        case REQUEST_LIST:
            ListRequest* listReq = (ListRequest*)request;
            memcpy(Path, listReq->path, MAX_PATH_LEN);
            break;
        case REQUEST_CREATE:
            CreateRequest* createReq = (CreateRequest*)request;
            memcpy(Path, createReq->path, MAX_PATH_LEN);
            break;
        case REQUEST_DELETE:
            DeleteRequest* deleteReq = (DeleteRequest*)request;
            memcpy(Path, deleteReq->path, MAX_PATH_LEN);
            break;
        default:
    }
}

bool isModified(RequestType type) {
    switch (type) {
        case REQUEST_CREATE:
        case REQUEST_WRITE:
        case REQUEST_COPY_WRITE:
        case REQUEST_DELETE:
            return true;
        case REQUEST_METADATA:
        case REQUEST_READ:
        case REQUEST_COPY_READ:
        default:
    }
    return false;
}

void ModifySSCopy(SSInfo ssinfo, RequestType type, void* request) {
    SSInfo ssCopy1, ssCopy2;
    // intialize sscpy1 and sscpy2
    ssRequests* ssReqts = &namingServer.SsRequests;
    pthread_mutex_lock(&ssReqts->ssRequestsLock);
    addSSRequest(ssReqts, type, request, ssCopy1, ssCopy2);
    pthread_mutex_unlock(&ssReqts->ssRequestsLock);
}
ErrorCode Copyhandler(void* request) {
    CopyRequest* Request = (CopyRequest*)request;
    SSInfo ssDst, ssSrc;
    lockTrie();
    ErrorCode retSrc = search(Request->SrcPath, &ssSrc);
    if (retSrc != EPATHNOTFOUND) {
        readLockPath(Request->SrcPath);
    }
    ErrorCode retDst = search(Request->DestPath, &ssDst);
    if (retDst != EPATHNOTFOUND) {
        writeLockPath(Request->DestPath);
    }
    unlockTrie();
    if (retDst == EPATHNOTFOUND || retSrc == EPATHNOTFOUND) {
        if (retSrc != EPATHNOTFOUND) readUnlockPath(Request->SrcPath);
        if (retDst != EPATHNOTFOUND) writeUnlockPath(Request->DestPath);
        return EPATHNOTFOUND;
    }

    lprintf("CopyHandler : ssSrc ssClientPort = %d, ssPassivePort = %d", ssSrc.ssClientPort, ssSrc.ssPassivePort);
    lprintf("CopyHandler : ssDst ssClientPort = %d, ssPassivePort = %d", ssDst.ssClientPort, ssDst.ssPassivePort);

    int sockDst, sockSrc;
    if (createActiveSocket(&sockSrc)) {
        if (retSrc != EPATHNOTFOUND) readUnlockPath(Request->SrcPath);
        if (retDst != EPATHNOTFOUND) writeUnlockPath(Request->DestPath);
        eprintf("Could not Create Active Socket(for Src)");
        return FAILURE;
    }
    lprintf("CopyHandler: Created active socket for src");

    if (connectToServer(sockSrc, ssSrc.ssClientPort)) {
        if (retSrc != EPATHNOTFOUND) readUnlockPath(Request->SrcPath);
        if (retDst != EPATHNOTFOUND) writeUnlockPath(Request->DestPath);
        eprintf("Could not connect to Src SS");
        return FAILURE;
    }

    SSCopyRequest readReq, writeReq;
    strcpy(readReq.path, Request->SrcPath);
    readReq.reqType = REQUEST_COPY_READ;

    lprintf("CopyHandler: Connected to src");

    sendRequestType(&readReq.reqType, sockSrc);
    lprintf("CopyHandler: Sent Request type to src");
    RequestTypeAck srcAck;
    bool srcRecvd;
    recieveRequestTypeAck(&srcAck, sockSrc, 30000, &srcRecvd);
    lprintf("CopyHandler: Recieved req type ack from src");

    sendRequest(readReq.reqType, &readReq, sockSrc);
    lprintf("CopyHandler: Sent Request to src");

    if (createActiveSocket(&sockDst)) {
        if (retSrc != EPATHNOTFOUND) readUnlockPath(Request->SrcPath);
        if (retDst != EPATHNOTFOUND) writeUnlockPath(Request->DestPath);
        eprintf("Could not Create Active Socket(for Dst)");
        return FAILURE;
    }
    lprintf("CopyHandler: Created active socket for dest");

    if (connectToServer(sockDst, ssDst.ssClientPort)) {
        if (retSrc != EPATHNOTFOUND) readUnlockPath(Request->SrcPath);
        if (retDst != EPATHNOTFOUND) writeUnlockPath(Request->DestPath);
        eprintf("Could not connect to Dst SS");
        return FAILURE;
    }

    lprintf("CopyHandler: Connected to dest");

    //  CODE WITHOUT ERROR HANDING  /////////
    strcpy(writeReq.path, Request->DestPath);
    writeReq.reqType = REQUEST_COPY_WRITE;

    sendRequestType(&writeReq.reqType, sockDst);
    lprintf("CopyHandler: Sent Request type to dest");

    RequestTypeAck destAck;
    bool destRecvd;
    recieveRequestTypeAck(&destAck, sockDst, 30000, &destRecvd);
    lprintf("CopyHandler: Recieved req type ack from dest");

    sendRequest(writeReq.reqType, &writeReq, sockDst);
    lprintf("CopyHandler: Sent Request to dest");

    CopyPacket packet;
    ReceivePacket(sockSrc, &packet);
    lprintf("CopyHandler: Recieved packet");
    while (packet.header != END) {
        if (packet.header == INFO) {
            char p[MAX_PATH_LEN];
            sprintf(p, "%s/%s", Request->DestPath, packet.ObjectInfo.FullName);
            lockTrie();
            addToTrie(p, ssDst);
            unlockTrie();
        }
        SendPacket(sockDst, &packet);
        lprintf("CopyHandler: Send packet");
        if (packet.header == STOP) {
            FeedbackAck ack;
            recieveFeedbackAck(&ack, sockDst);
            lprintf("file written successfully");
            sendFeedbackAck(&ack, sockSrc);
        }

        ReceivePacket(sockSrc, &packet);
        lprintf("CopyHandler: Recieved packet");
    }

    lprintf("CopyHandler: Routing finished");
    if (retSrc != EPATHNOTFOUND) readUnlockPath(Request->SrcPath);
    if (retDst != EPATHNOTFOUND) writeUnlockPath(Request->DestPath);
    FeedbackAck SrcAck, DstAck;
    recieveFeedbackAck(&SrcAck, sockSrc);
    recieveFeedbackAck(&DstAck, sockDst);
    close(sockSrc);
    close(sockDst);
    if (SrcAck.errorCode == SUCCESS && DstAck.errorCode == SUCCESS) {
        if (isModified(writeReq.reqType)) {
            ModifySSCopy(ssDst, writeReq.reqType, request);
        }
        return SUCCESS;
    }
    return FAILURE;
}

ErrorCode handleListRequest(void* rawRequest, ConnectedClient client) {
    ListRequest* castRequest = (ListRequest*)rawRequest;
    lprintf("Client Thread (Alive port = %d) : Processing list request with path %s", client->clientInitRequest.clientAlivePort, castRequest->path);
    ListResponse response;
    response.list_cnt = 0;
    SSInfo temp;
    char** ret = NULL;
    lockTrie();
    if (search(castRequest->path, &temp) != EPATHNOTFOUND) {
        readLockPath(castRequest->path);
        ret = getChildren(castRequest->path, &response.list_cnt);
        readUnlockPath(castRequest->path);
    }
    unlockTrie();

    for (int i = 0; i < response.list_cnt; i++) {
        strcpy(response.list[i], ret[i]);
    }
    if (ret) {
        for (int i = 0; i < response.list_cnt; i++) free(ret[i]);
        free(ret);
    }

    if (socketSend(client->clientSockfd, &response, sizeof(ListResponse))) {
        eprintf("Client Thread (Alive port = %d) : Could not send ListResponse", client->clientInitRequest.clientAlivePort);
        free(rawRequest);
        return FAILURE;
    }
    FeedbackAck ack;
    ack.errorCode = 0;
    if (ret == NULL) ack.errorCode = EPATHNOTFOUND;
    if (sendFeedbackAck(&ack, client->clientSockfd)) {
        eprintf("Client Thread (Alive port = %d) : Could not send Feedback Ack", client->clientInitRequest.clientAlivePort);
        free(rawRequest);
        return FAILURE;
    }
    free(rawRequest);
    return SUCCESS;
}

ErrorCode handleNP(RequestType type, void* rawRequest, ConnectedClient client) {
    char RequestPath[MAX_PATH_SIZE];
    ExtractPath(rawRequest, type, RequestPath);
    lprintf("Client Thread (Alive port = %d) : Processing NP request with path %s", client->clientInitRequest.clientAlivePort, RequestPath);
    SSInfo ssinfo;
    lockTrie();
    ErrorCode ret = search(RequestPath, &ssinfo);
    if (ret != EPATHNOTFOUND) {
        if (type == REQUEST_WRITE) {
            writeLockPath(RequestPath);
        } else {
            readLockPath(RequestPath);
        }
    }
    unlockTrie();
    lprintf("Client Thread (Alive port = %d) : ssclientInfo = %d, sspassiveport = %d", client->clientInitRequest.clientAlivePort, ssinfo.ssClientPort, ssinfo.ssPassivePort);
    if (sendSSInfo(&ssinfo, client->clientSockfd)) {
        eprintf("Could not send SSinfo to client");
        if (ret != EPATHNOTFOUND) {
            if (type == REQUEST_WRITE) {
                writeUnlockPath(RequestPath);
            } else {
                readUnlockPath(RequestPath);
            }
        }
        free(rawRequest);
        return FAILURE;
    }

    FeedbackAck ack;
    if (recieveFeedbackAck(&ack, client->clientSockfd)) {
        eprintf("Could not receive feedback from client\n");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) {
            if (type == REQUEST_WRITE) {
                writeUnlockPath(RequestPath);
            } else {
                readUnlockPath(RequestPath);
            }
        }
        return FAILURE;
    }
    lprintf("FeedbackAck received from client : %d", ack.errorCode);
    if (ret != EPATHNOTFOUND) {
        if (type == REQUEST_WRITE) {
            writeUnlockPath(RequestPath);
        } else {
            readUnlockPath(RequestPath);
        }
    }
    lprintf("FeedbackAck received from client : %d", ack.errorCode);
    if (ack.errorCode == SUCCESS && isModified(type)) {
        ModifySSCopy(ssinfo, type, rawRequest);
    }
    free(rawRequest);
    return SUCCESS;
}

ErrorCode handleCreateRequest(void* rawRequest, ConnectedClient client) {
    ErrorCode r = SUCCESS;
    CreateRequest* castRequest = (CreateRequest*)rawRequest;
    lprintf("Client Thread (Alive port = %d) : Processing Create request with path %s", client->clientInitRequest.clientAlivePort, castRequest->path);
    SSInfo ssinfo;
    lockTrie();
    ErrorCode ret = search(castRequest->path, &ssinfo);
    if (ret != EPATHNOTFOUND) {
        readLockPath(castRequest->path);
    }
    unlockTrie();

    if (ret == EPATHNOTFOUND) {
        lprintf("Client Thread (Alive port = %d) : Did not find path %s", client->clientInitRequest.clientAlivePort, castRequest->path);
        r = getBestSS(&ssinfo);
        if (r == ENO_SS) {
            lprintf("Client Thread (Alive port = %d) : No storage servers available", client->clientInitRequest.clientAlivePort);
            FeedbackAck ack;
            ack.errorCode = r;
            if (sendFeedbackAck(&ack, client->clientSockfd)) {
                eprintf("Client Thread (Alive port = %d) : Could not send Feedback Ack\n", client->clientInitRequest.clientAlivePort);
                return FAILURE;
            }
            lprintf("Got best ss ssClientport =  %d, ssPassivePort = %d", ssinfo.ssClientPort, ssinfo.ssPassivePort);
            return SUCCESS;
        }
    }

    int sockfd;
    if (createActiveSocket(&sockfd)) {
        eprintf("Could not Create Active Socket\n");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Active Socket Created for create", client->clientInitRequest.clientAlivePort);

    if (connectToServer(sockfd, ssinfo.ssClientPort)) {
        eprintf("Could not connect to SS\n");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Connected to SS for create", client->clientInitRequest.clientAlivePort);
    RequestType type = REQUEST_CREATE;
    if (sendRequestType(&type, sockfd)) {
        eprintf("Could not send requestType");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : RequestType sent", client->clientInitRequest.clientAlivePort);
    RequestTypeAck reqTypeAck;
    bool recvd;
    if (recieveRequestTypeAck(&reqTypeAck, sockfd, 30000, &recvd)) {
        eprintf("Could not recieve request type ack\n");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Recieved request type ack", client->clientInitRequest.clientAlivePort);

    if (sendRequest(REQUEST_CREATE, rawRequest, sockfd)) {
        eprintf("Could not send Request\n");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Request sent for create", client->clientInitRequest.clientAlivePort);

    FeedbackAck ack;
    if (recieveFeedbackAck(&ack, sockfd)) {
        eprintf("Could not recieve FeedbackAck from SS\n");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : FeedbackAck Recieved for create", client->clientInitRequest.clientAlivePort);
    char fullPath[MAX_PATH_LEN];
    strcpy(fullPath, castRequest->path);
    if (!castRequest->isDirectory) {
        strcat(fullPath, "/");
        strcat(fullPath, castRequest->newObject);
    }
    if (ack.errorCode == SUCCESS) {
        lockTrie();
        addToTrie(fullPath, ssinfo);
        unlockTrie();
    }

    if (ack.errorCode == SUCCESS && isModified(type)) {
        ModifySSCopy(ssinfo, type, rawRequest);
    }

    if (sendFeedbackAck(&ack, client->clientSockfd)) {
        eprintf("Could not send FeedbackAck to client\n");
        free(rawRequest);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : FeedbackAck sent(to client) for create", client->clientInitRequest.clientAlivePort);

    if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
    free(rawRequest);
    close(sockfd);
    return r;
}

ErrorCode handleDeleteRequest(void* rawRequest, ConnectedClient client) {
    ErrorCode r = SUCCESS;
    DeleteRequest* castRequest = (DeleteRequest*)rawRequest;
    lprintf("Client Thread (Alive port = %d) : Processing Delete request with path %s", client->clientInitRequest.clientAlivePort, castRequest->path);
    SSInfo ssinfo;
    lockTrie();
    ErrorCode ret = search(castRequest->path, &ssinfo);
    if (ret != EPATHNOTFOUND) {
        readLockPath(castRequest->path);
    }
    unlockTrie();

    if (ret == EPATHNOTFOUND) {
        lprintf("Client Thread (Alive port = %d) : Did not find path %s", client->clientInitRequest.clientAlivePort, castRequest->path);
        FeedbackAck ack;
        ack.errorCode = EPATHNOTFOUND;
        if (sendFeedbackAck(&ack, client->clientSockfd)) {
            eprintf("Could not send FeedbackAck to client\n");
            free(rawRequest);
            return FAILURE;
        }
        return FAILURE;
    }

    int sockfd;
    if (createActiveSocket(&sockfd)) {
        eprintf("Could not Create Active Socket\n");
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        free(rawRequest);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Active Socket Created for delete", client->clientInitRequest.clientAlivePort);

    if (connectToServer(sockfd, ssinfo.ssClientPort)) {
        eprintf("Could not connect to SS\n");
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        free(rawRequest);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Connected to SS for delete", client->clientInitRequest.clientAlivePort);
    RequestType type = REQUEST_DELETE;
    if (sendRequestType(&type, sockfd)) {
        eprintf("Could not send requestType");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : RequestType sent", client->clientInitRequest.clientAlivePort);

    RequestTypeAck reqTypeAck;
    bool recvd;
    if (recieveRequestTypeAck(&reqTypeAck, sockfd, 30000, &recvd)) {
        eprintf("Could not recieve request type ack\n");
        free(rawRequest);
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Recieved request type ack", client->clientInitRequest.clientAlivePort);

    if (sendRequest(REQUEST_DELETE, rawRequest, sockfd)) {
        eprintf("Could not send Request\n");
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        free(rawRequest);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : Request sent for delete", client->clientInitRequest.clientAlivePort);

    FeedbackAck ack;
    if (recieveFeedbackAck(&ack, sockfd)) {
        eprintf("Could not recieve FeedbackAck from SS\n");
        if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);
        free(rawRequest);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : FeedbackAck Recieved for delete", client->clientInitRequest.clientAlivePort);

    if (ack.errorCode == SUCCESS) {
        lockTrie();
        printf("%d\n", deleteFromTrie(castRequest->path));
        unlockTrie();
    }

    if (ack.errorCode == SUCCESS && isModified(type)) {
        ModifySSCopy(ssinfo, type, rawRequest);
    }

    if (sendFeedbackAck(&ack, client->clientSockfd)) {
        eprintf("Could not send FeedbackAck to client\n");
        free(rawRequest);
        close(sockfd);
        return FAILURE;
    }

    lprintf("Client Thread (Alive port = %d) : FeedbackAck sent(to client) for create", client->clientInitRequest.clientAlivePort);
    if (ret != EPATHNOTFOUND) readUnlockPath(castRequest->path);

    free(rawRequest);
    close(sockfd);
    return r;
}
