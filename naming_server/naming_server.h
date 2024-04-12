#ifndef __NAMING_SERVER_H
#define __NAMING_SERVER_H

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../common/error/error.h"
#include "../common/networking/nm_client/client_connect.h"
#include "../common/networking/nm_ss/ss_connect.h"
#include "../common/print/logging.h"
#include "../common/signals/cleanup_signal.h"
#include "./threads/for_client/client_alive_thread.h"
#include "./threads/for_client/client_listener_thread.h"
#include "./threads/for_client/connected_clients.h"
#include "./threads/for_ss/ssReq_handler.h"
#include "./threads/for_ss/ss_alive_thread.h"
#include "./threads/for_ss/ss_listener_thread.h"
#include "./trie/trie.h"

#define FATAL_EXIT   \
    do {             \
        destroyNM(); \
        exit(1);     \
    } while (0)

#define UNUSED(x) \
    do {          \
        (void)x;  \
    } while (0);

#define MAX_STORAGE_SERVERS 100

typedef struct ConnectedSS {
    int count;
    SSInitRequest storageServers[MAX_STORAGE_SERVERS];
    int storageServerSockfds[MAX_STORAGE_SERVERS];
} ConnectedSS;

typedef struct SSMap {
    int uniqueSSCount; /* Number of unique SS's connected so far*/
    char cwds[MAX_STORAGE_SERVERS][MAX_PATH_LEN];
    SSInfo ssinfos[MAX_STORAGE_SERVERS];
    SSInfo redCopy[MAX_STORAGE_SERVERS][2];
    bool readOnly[MAX_STORAGE_SERVERS];
    bool online[MAX_STORAGE_SERVERS];
} SSMap;

typedef struct NamingServer {
    int ssListenerSockfd;            /* Socket for ss listener passive port */
    ConnectedSS connectedSS;         /* Stores connected storage servers, must be locked for synchronization */
    pthread_mutex_t connectedSSLock; /* Lock for connectedSS */
    pthread_t ssListener;            /* Storage server listener thread */
    pthread_t ssAliveChecker;        /* Checks if the connected ss's are alive */

    int clientListenerSockfd;          /* Socket for client listener passive port */
    ConnectedClients connectedClients; /* Stores connected clients, must be locked for synchronization */
    pthread_t clientListener;          /* Client listener thread */
    pthread_t clientAliveChecker;      /* Checks if connected clients are alive */

    int ssReqListenerSockfd; /* SS request Listener socket*/
    pthread_t ssReqListner;
    ssRequests SsRequests;

    pthread_mutex_t mapLock;
    SSMap map;

    /* Cleanup stuff */
    sig_atomic_t isCleaningUp;
    ErrorCode exitCode;
} NamingServer;

/* Unique naming server instance */
extern NamingServer namingServer;

ErrorCode initNM();
void destroyNM();
bool isCleaningUp();
void initiateCleanup(ErrorCode exitCode);
ErrorCode getBestSS(SSInfo* ret);

int getSSIndexFromCwd(char* cwd);
int getSSIndexFromInfo(SSInfo* ssinfo);
ErrorCode getRedundantSSInfo(SSInfo* input, SSInfo* s1, SSInfo* s2);
bool isReadOnly(SSInfo* ssinfo);
bool isOnline(SSInfo* ssinfo);
#endif