#include "ss_alive_thread.h"
#include <assert.h>
#include "../../../common/networking/networking.h"

/* Termicnates naming server in case of fatal errors */
void* ssAliveRoutine(void* arg) {
    UNUSED(arg);
    ConnectedSS* connectedSS = &namingServer.connectedSS;
    int toRemoveIndices[MAX_STORAGE_SERVERS];

    while (!isCleaningUp()) {
        int toRemoveCount = 0;
        pthread_mutex_lock(&namingServer.connectedSSLock);
        for (int i = 0; i < connectedSS->count; i++) {
            int ssAlivePort = connectedSS->storageServers[i].SSAlivePort;

            int tempSockfd;
            if (createActiveSocket(&tempSockfd)) {
                eprintf("Could not create active socket to test if storage server with alive port %d is alive\n", ssAlivePort);
                FATAL_EXIT;
            }

            if (!canConnectToServer(tempSockfd, ssAlivePort)) {
                lprintf("SS_Alive : Storage server with alive port %d disconnected", ssAlivePort);
                toRemoveIndices[toRemoveCount++] = i;
            }

            if (closeSocket(tempSockfd)) {
                eprintf("Could not close socket\n");
                FATAL_EXIT;
            }
        }

        int newSSCount = 0;
        int ptr = 0;
        for (int i = 0; i < connectedSS->count; i++) {
            SSInitRequest* req = &connectedSS->storageServers[i];
            if (ptr < toRemoveCount && i == toRemoveIndices[ptr]) {  // ignore i
                ptr++;
                SSInfo ssinfo;
                initSSInfo(&ssinfo, req->SSClientPort, req->SSPassivePort);
                lockTrie();
                deleteSSFromTrie(ssinfo);
                unlockTrie();
                
                int idx = getSSIndexFromInfo(&ssinfo);
                assert(idx != -1);
                pthread_mutex_lock(&namingServer.mapLock);
                namingServer.map.readOnly[idx] = true;
                namingServer.map.online[idx] = false;
                pthread_mutex_unlock(&namingServer.mapLock);
            } else {
                connectedSS->storageServers[newSSCount] = connectedSS->storageServers[i];
                connectedSS->storageServerSockfds[newSSCount] = connectedSS->storageServerSockfds[i];
                newSSCount++;
            }
        }

        connectedSS->count = newSSCount;
        pthread_mutex_unlock(&namingServer.connectedSSLock);
    }

    lprintf("SS_Alive : Cleaning up");
    return NULL;
}

ErrorCode createSSAliveThread(pthread_t* aliveThread) {
    int ret;
    if ((ret = pthread_create(aliveThread, NULL, ssAliveRoutine, NULL))) {
        eprintf("Could not create storage server alive thread, errno = %d, %s\n", ret, strerror(ret));
        return FAILURE;
    }

    return SUCCESS;
}