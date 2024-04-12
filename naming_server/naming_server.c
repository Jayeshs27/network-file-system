#include "naming_server.h"

#include <assert.h>
#include <sys/socket.h>

#include "../common/networking/networking.h"

NamingServer namingServer;

#define JOIN_IF_CREATED(T, RET)      \
    do {                             \
        if (T) pthread_join(T, RET); \
    } while (0)

ErrorCode initConnectedSS() {
    namingServer.connectedSS.count = 0;
    int ret;
    if ((ret = pthread_mutex_init(&namingServer.connectedSSLock, NULL))) {
        eprintf("Could not initialize connected storage server mutex, errno = %d, %s\n", ret, strerror(ret));
        return FAILURE;
    }

    return SUCCESS;
}

void destroyConnectedSS() {
    pthread_mutex_destroy(&namingServer.connectedSSLock);
}

void signalSuccess();

ErrorCode initNM() {
    namingServer.ssListener = 0;
    namingServer.ssAliveChecker = 0;
    namingServer.clientListener = 0;
    namingServer.clientAliveChecker = 0;
    namingServer.ssReqListner = 0;
    namingServer.isCleaningUp = 0;
    pthread_mutex_init(&namingServer.mapLock, NULL);
    namingServer.map.uniqueSSCount = 0;
    if (initLogger("logs/naming_server/", false)) {
        eprintf("Could not create log file\n");
        goto destroy_map_lock;
    }

    if (startLogging()) {
        eprintf("Could not start logging\n");
        goto destroy_logger;
    }

    lprintf("Main : Initializing trie");
    initTrie();

    initEscapeHatch(signalSuccess);

    if (initConnectedSS()) {
        goto destroy_escape_hatch;
    }

    lprintf("Main : Creating passive socket (ss listener) on port = %d", SS_LISTEN_PORT);
    if (createPassiveSocket(&namingServer.ssListenerSockfd, SS_LISTEN_PORT)) {
        goto destroy_connected_SS;
    }

    if (initConnectedClients(&namingServer.connectedClients)) {
        goto destroy_SS_listener;
    }

    lprintf("Main : Creating passive socket (client listener) on port = %d", CLIENT_LISTEN_PORT);
    if (createPassiveSocket(&namingServer.clientListenerSockfd, CLIENT_LISTEN_PORT)) {
        goto destroy_connected_clients;
    }

    if (createPassiveSocket(&namingServer.ssReqListenerSockfd, SS_REQ_LISTEN_PORT)) {
        goto destroy_ssReqlistener;
    }

    initSSRequests(&namingServer.SsRequests);

    return SUCCESS;

    // destroy_client_listener:
    //     close(namingServer.clientListenerSockfd);
destroy_ssReqlistener:
    close(namingServer.ssReqListenerSockfd);

destroy_connected_clients:
    destroyConnectedClients(&namingServer.connectedClients);

destroy_SS_listener:
    close(namingServer.ssListenerSockfd);

destroy_connected_SS:
    destroyConnectedSS();

destroy_escape_hatch:
    destroyEscapeHatch();
    endLogging();

destroy_logger:
    destroyLogger();
destroy_map_lock:
    pthread_mutex_destroy(&namingServer.mapLock);
    exit(FAILURE);
}

void destroyNM() {
    JOIN_IF_CREATED(namingServer.clientAliveChecker, NULL);
    JOIN_IF_CREATED(namingServer.ssAliveChecker, NULL);

    shutdown(namingServer.ssListenerSockfd, SHUT_RDWR);
    shutdown(namingServer.clientListenerSockfd, SHUT_RDWR);

    JOIN_IF_CREATED(namingServer.clientListener, NULL);
    JOIN_IF_CREATED(namingServer.ssListener, NULL);

    destroyConnectedClients(&namingServer.connectedClients);

    lprintf("Main : Cleaning up trie");
    destroyTrie();

    lprintf("Main : Exit request to logger thread");
    endLogging();
    destroyLogger();
    JOIN_IF_CREATED(getLoggingThread(), NULL);

    pthread_mutex_destroy(&namingServer.connectedSSLock);
    destroySSRequests(&namingServer.SsRequests);
    destroyEscapeHatch();
    exit(namingServer.exitCode);
}

bool isCleaningUp() {
    return namingServer.isCleaningUp;
}

void initiateCleanup(ErrorCode exitCode) {
    namingServer.exitCode = exitCode;
    namingServer.isCleaningUp = true;
}

void signalSuccess() {
    initiateCleanup(SUCCESS);
    destroyNM();
}

ErrorCode getBestSS(SSInfo* ret) {
    ErrorCode r = SUCCESS;
    pthread_mutex_lock(&namingServer.connectedSSLock);
    if (namingServer.connectedSS.count == 0) {
        r = FAILURE, *ret = INVALID_SSINFO;
    } else {
        ret->ssClientPort = namingServer.connectedSS.storageServers[0].SSClientPort;
        ret->ssPassivePort = namingServer.connectedSS.storageServers[0].SSPassivePort;
    }
    pthread_mutex_unlock(&namingServer.connectedSSLock);
    return r;
}

int getSSIndexFromCwd(char* cwd) {
    SSMap* map = &namingServer.map;
    for (int i = 0; i < map->uniqueSSCount; i++) {
        if (strcmp(map->cwds[i], cwd) == 0) {
            pthread_mutex_unlock(&namingServer.mapLock);
            return i;
        }
    }
    return -1;
}

int getSSIndexFromInfo(SSInfo* ssinfo) {
    SSMap* map = &namingServer.map;

    for (int i = 0; i < map->uniqueSSCount; i++) {
        if (SSInfoEqual(&map->ssinfos[i], ssinfo)) {
            pthread_mutex_unlock(&namingServer.mapLock);
            return i;
        }
    }
}

ErrorCode getRedundantSSInfo(SSInfo* input, SSInfo* s1, SSInfo* s2) {
    *s1 = INVALID_SSINFO;
    *s2 = INVALID_SSINFO;
    SSMap* map = &namingServer.map;

    for (int i = 0; i < map->uniqueSSCount; i++) {
        if (SSInfoEqual(&map->ssinfos[i], input)) {
            *s1 = map->redCopy[i][0];
            *s2 = map->redCopy[i][1];
            pthread_mutex_unlock(&namingServer.mapLock);
            return SUCCESS;
        }
    }
    return ENO_SS;
}

bool isReadOnly(SSInfo* ssinfo) {
    SSMap* map = &namingServer.map;
    for (int i = 0; i < map->uniqueSSCount; i++) {
        if (SSInfoEqual(&map->ssinfos[i], ssinfo)) {
            bool ret = map->readOnly[i];
            pthread_mutex_unlock(&namingServer.mapLock);
            return ret;
        }
    }
}

bool isOnline(SSInfo* ssinfo) {
    SSMap* map = &namingServer.map;
    pthread_mutex_lock(&namingServer.mapLock);
    for (int i = 0; i < map->uniqueSSCount; i++) {
        if (SSInfoEqual(&map->ssinfos[i], ssinfo)) {
            bool ret = map->online[i];
            pthread_mutex_unlock(&namingServer.mapLock);
            return ret;
        }
    }
    pthread_mutex_unlock(&namingServer.mapLock);
}