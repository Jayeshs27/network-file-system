#include "client_alive_thread.h"

#include "../../../common/networking/networking.h"

/* Termicnates naming server in case of fatal errors */
void* clientAliveRoutine(void* arg) {
    UNUSED(arg);
    ConnectedClients* connectedClients = &namingServer.connectedClients;

    while (!isCleaningUp()) {
        pthread_mutex_lock(&connectedClients->clientLock);
        for (ConnectedClient itr = connectedClients->front, nxt; itr != NULL; itr = nxt) {
            nxt = itr->next;
            int clientAlivePort = itr->clientInitRequest.clientAlivePort;

            int tempSockfd; 
            if (createActiveSocket(&tempSockfd)) {
                eprintf("Could not create active socket to test if client with alive port %d is alive\n", clientAlivePort);
                initiateCleanup(FAILURE);
            }

            // lprintf("Client_Alive : Checking if client is alive");

            if (!canConnectToServer(tempSockfd, clientAlivePort)) {
                lprintf("Client_Alive: Client with alive port %d disconnected", clientAlivePort);
                removeClient(connectedClients, itr);
                // cleanly destroy the associated thread
                free(itr);
            }

            // lprintf("Client_Alive : Client is alive");

            if (closeSocket(tempSockfd)) {
                eprintf("Could not close socket\n");
                initiateCleanup(FAILURE);
            }

            // lprintf("Client_Alive : Closed socket");

        }
        pthread_mutex_unlock(&connectedClients->clientLock);
    }

    lprintf("Client_Alive : Cleaning up");
    return NULL;
}

ErrorCode createClientAliveThread(pthread_t* aliveThread) {
    int ret;
    if ((ret = pthread_create(aliveThread, NULL, clientAliveRoutine, NULL))) {
        eprintf("Could not create client alive thread, errno = %d, %s\n", ret, strerror(ret));
        return FAILURE;
    }

    return SUCCESS;
}