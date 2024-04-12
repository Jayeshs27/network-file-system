#include "connected_clients.h"
#include "client_handler_thread.h"
#include <assert.h>
#include <stdlib.h>

ConnectedClient newConnectedClient(ClientInitRequest* clientInitRequest, int clientSockfd) {
    ConnectedClient ret = calloc(1, sizeof(ConnectedClientStruct));
    assert(ret);
    ret->clientInitRequest = *clientInitRequest;
    ret->clientSockfd = clientSockfd;
    ret->isCleaningup = false;
    pthread_mutex_init(&ret->cleanupLock, NULL);
    return ret;
}

void destroyConnectedClient(ConnectedClient client) {
    pthread_mutex_destroy(&client->cleanupLock);
    free(client);
}

ErrorCode initConnectedClients(ConnectedClients* connectedClients) {
    connectedClients->front = connectedClients->rear = NULL;
    connectedClients->count = 0;
    if (pthread_mutex_init(&connectedClients->clientLock, NULL)) {
        eprintf("Could not initialize connectedClients mutex");
        return FAILURE;
    }

    return SUCCESS;
}

void destroyConnectedClients(ConnectedClients* connectedClients) {
    pthread_mutex_destroy(&connectedClients->clientLock);
    for (ConnectedClient itr = connectedClients->front; itr != NULL;) {
        ConnectedClient nxt = itr->next;
        destroyConnectedClient(itr);
        itr = nxt;
    }
}

static void Link(ConnectedClient a, ConnectedClient b) {
    if (a) a->next = b;
    if (b) b->prev = a;
}

ErrorCode addClient(ConnectedClients* connectedClients, ClientInitRequest* req, int clientSockfd) {
    assert(connectedClients->count >= 0);
    ConnectedClient newClient = newConnectedClient(req, clientSockfd);
    Link(connectedClients->rear, newClient);
    connectedClients->rear = newClient;
    connectedClients->count++;
    if (connectedClients->front == NULL) connectedClients->front = connectedClients->rear;
    startClientThread(newClient);
    return SUCCESS;
}

ErrorCode removeClient(ConnectedClients* connectedClients, ConnectedClient client) {
    assert(connectedClients->count > 0);
    assert(client);
    connectedClients->count--;
    ConnectedClient front = connectedClients->front;
    ConnectedClient rear = connectedClients->rear;

    if (client == front) connectedClients->front = front->next;
    if (client == rear) connectedClients->rear = rear->prev;
    Link(client->prev, client->next);
    killClientThread(client);
    printf("killed!\n");
    pthread_join(client->thread, NULL);
    return SUCCESS;
}