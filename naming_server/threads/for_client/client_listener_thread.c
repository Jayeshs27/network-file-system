#include "client_listener_thread.h"

#include "../../../common/networking/networking.h"

/* Terminates naming server in case of fatal errors */
void* clientListenerRoutine(void* arg) {
    UNUSED(arg);
    ConnectedClients* connectedClients = &namingServer.connectedClients;
    while (!isCleaningUp()) {
        lprintf("Client_listener : Waiting for client...");

        int clientSockfd;
        if (acceptClient(namingServer.clientListenerSockfd, &clientSockfd)) {
            initiateCleanup(SUCCESS);
            break;
        }

        lprintf("Client_listener : Client connected");
        ClientInitRequest recievedReq;
        if (recieveClientRequest(clientSockfd, &recievedReq)) {
            initiateCleanup(SUCCESS);
            break;
        }

        lprintf("Client_listener : Recieved Passive port = %d, Alive port = %d", recievedReq.clientPassivePort, recievedReq.clientAlivePort);

        // printf("Client_listener trying to lock connectedClientsLock\n");
        pthread_mutex_lock(&connectedClients->clientLock);
        addClient(connectedClients, &recievedReq, clientSockfd);
        pthread_mutex_unlock(&connectedClients->clientLock);
        // printf("Client_listener trying to lock cleanup\n");
    }

    lprintf("Client_listener : Cleaning up");
    close(namingServer.clientListenerSockfd);
    return NULL;
}

ErrorCode createClientListenerThread(pthread_t* listenerThread) {
    int ret;
    if ((ret = pthread_create(listenerThread, NULL, clientListenerRoutine, NULL))) {
        eprintf("Could not create client listener thread, errno = %d, %s\n", ret, strerror(ret));
        return FAILURE;
    }

    return SUCCESS;
}

