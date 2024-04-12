#ifndef __CONNECTED_CLIENTS_H
#define __CONNECTED_CLIENTS_H

#include <pthread.h>
#include <stdbool.h>

#include "../../../common/error/error.h"
#include "../../../common/networking/nm_client/client_connect.h"

typedef struct ConnectedClientStruct {
    ClientInitRequest clientInitRequest;
    int clientSockfd;
    pthread_t thread;

    /* Requires cleanupLock to be held */
    pthread_mutex_t cleanupLock;
    bool isCleaningup;

    struct ConnectedClientStruct* next;
    struct ConnectedClientStruct* prev;
} ConnectedClientStruct;

typedef ConnectedClientStruct* ConnectedClient;

typedef struct ConnectedClients {
    int count;
    ConnectedClient front;
    ConnectedClient rear;
    pthread_mutex_t clientLock;
} ConnectedClients;

ErrorCode initConnectedClients(ConnectedClients* connectedClients);
void destroyConnectedClients(ConnectedClients* connectedClients);

/* Requires cleanupLock to be held */
ErrorCode addClient(ConnectedClients* connectedClients, ClientInitRequest* req, int clientSockfd);

/* Requires cleanupLock to be held */
ErrorCode removeClient(ConnectedClients* connectedClients, ConnectedClient client);

#endif