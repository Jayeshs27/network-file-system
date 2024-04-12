#include "ss_listener_thread.h"

#include "../../../common/networking/networking.h"

void addSS(ConnectedSS* connectedSS, SSInitRequest* req, int ssSockfd) {
    connectedSS->storageServers[connectedSS->count] = *req;
    connectedSS->storageServerSockfds[connectedSS->count] = ssSockfd;
    connectedSS->count++;
    
    SSInfo ssinfo;
    initSSInfo(&ssinfo, req->SSClientPort, req->SSPassivePort);
    lockTrie();
    for (int i = 0; i < req->paths.count; i++) {
        addToTrie(req->paths.pathList[i], ssinfo);
    }
    unlockTrie();

    SSMap* map = &namingServer.map;

    int indx = getSSIndexFromCwd(req->cwd);
    // pthread_mutex_lock(&namingServer.mapLock);
    if (indx == -1) {
        int cnt = map->uniqueSSCount;
        strcpy(map->cwds[cnt], req->cwd);
        map->ssinfos[cnt].ssClientPort = req->SSClientPort;
        map->ssinfos[cnt].ssPassivePort = req->SSPassivePort;
        map->redCopy[cnt][0] = map->redCopy[cnt][1] = INVALID_SSINFO;
        map->readOnly[cnt] = false;
        map->online[cnt] = true;
        map->uniqueSSCount++;
        lprintf("number of uniqueSS's %d",map->uniqueSSCount);
        if (map->uniqueSSCount > 2) {
            if (map->uniqueSSCount == 3) {
                SSInfo s1,s2;
                map->redCopy[0][0] = map->ssinfos[1];
                map->redCopy[0][1] = map->ssinfos[2];
                map->redCopy[1][0] = map->ssinfos[0];
                map->redCopy[1][1] = map->ssinfos[2];
                RequestType type = REQUEST_COPY_WRITE;
                SSCopyRequest req1,req2;
                req1.reqType = REQUEST_COPY_WRITE;
                req2.reqType = REQUEST_COPY_WRITE;
                strcpy(req1.otherpath, map->cwds[0]);
                addSSRequest(&namingServer.SsRequests,type,&req1,map->ssinfos[1],map->ssinfos[2]);
                strcpy(req2.otherpath, map->cwds[1]);
                addSSRequest(&namingServer.SsRequests,type,&req2,map->ssinfos[0],map->ssinfos[2]);
            }
            map->redCopy[cnt][0] = map->ssinfos[0];
            map->redCopy[cnt][1] = map->ssinfos[1];
            RequestType type = REQUEST_COPY_WRITE;
            SSCopyRequest req3;
            req3.reqType = REQUEST_COPY_WRITE;
            strcpy(req3.otherpath, map->cwds[cnt]);
            addSSRequest(&namingServer.SsRequests,type,&req3,map->ssinfos[1],map->ssinfos[2]);
            // initialize this new ss
            // setup threads to duplicate data in ss1 and ss2
        }
    } else {
        // ss reconnected
        map->online[indx] = true;
        map->readOnly[indx] = false;
    }
    // pthread_mutex_unlock(&namingServer.mapLock);
}

/* Terminates naming server in case of fatal errors */
void* ssListenerRoutine(void* arg) {
    UNUSED(arg);
    ConnectedSS* connectedSS = &namingServer.connectedSS;
    while (!isCleaningUp()) {
        lprintf("SS_listener : Waiting for storage server...");
        int ssSockfd;
        if (acceptClient(namingServer.ssListenerSockfd, &ssSockfd)) {
            initiateCleanup(SUCCESS);
            break;
        }

        lprintf("SS_listener : Storage server connected");
        SSInitRequest recievedReq;
        if (recieveSSRequest(ssSockfd, &recievedReq)) {
            initiateCleanup(SUCCESS);
            break;
        }

        lprintf("SS_listener : cwd :%s  Recieved Alive port = %d, Passive port = %d, Client port = %d",recievedReq.cwd, recievedReq.SSAlivePort, recievedReq.SSPassivePort, recievedReq.SSClientPort);

        printf("path count = %d\n", recievedReq.paths.count);
        // for (int i = 0; i < recievedReq.paths.count; i++) {
        //     printf("\t%s\n", recievedReq.paths.pathList[i]);
        // }

        // printf("ssListener trying to lock connectedSSLock\n");
        pthread_mutex_lock(&namingServer.connectedSSLock);
        addSS(connectedSS, &recievedReq, ssSockfd);
        if (connectedSS->count > MAX_STORAGE_SERVERS) {
            eprintf("Too many storage servers\n");
            pthread_mutex_unlock(&namingServer.connectedSSLock);
            initiateCleanup(SUCCESS);
            break;
        }
        pthread_mutex_unlock(&namingServer.connectedSSLock);
    }

    // cleaning up
    lprintf("SS_listener : Cleaning up");
    close(namingServer.ssListenerSockfd);
    return NULL;
}

ErrorCode createSSListenerThread(pthread_t* listenerThread) {
    int ret;
    if ((ret = pthread_create(listenerThread, NULL, ssListenerRoutine, NULL))) {
        eprintf("Could not create storage server listener thread, errno = %d, %s\n", ret, strerror(ret));
        return FAILURE;
    }

    return SUCCESS;
}