#ifndef __CLIENT_H
#define __CLIENT_H

#include <pthread.h>
#include <signal.h>

#include "../common/error/error.h"
#include "../common/networking/networking.h"
#include "../common/networking/nm_client/client_connect.h"
#include "../common/networking/requests.h"
#include "../common/print/logging.h"
#include "../common/signals/cleanup_signal.h"
#include "../common/threads/alive_socket_thread.h"
#include "client_requests.h"
#include "../common/print/print.h"

typedef struct Client {
    int passiveSockfd;   /* Temporary */
    int passiveSockPort; /* Temporary */

    int nmSockfd; /* To initialize & communicate with nm */
    AliveSocketThread thread;

    /* Cleanup stuff */
    sig_atomic_t isCleaningup;
    ErrorCode exitCode;
} Client;

extern Client client;

ErrorCode initClient();
void destroyClient();
ErrorCode connectToNM();

/* The pointer returned must be destroyed */
void* inputRequest(RequestType* requestType);
void destroyRequest(void* request);

#define TIMEOUT_MILLIS 20000

ErrorCode inputAndSendRequest();
void signalSuccess();
void initiateCleanup(ErrorCode exitCode);

#endif