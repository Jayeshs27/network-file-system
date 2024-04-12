#ifndef __STORAGE_SERVER_H
#define __STORAGE_SERVER_H

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>

#include "../common/error/error.h"
#include "../common/networking/networking.h"
#include "../common/networking/nm_ss/ss_connect.h"
#include "../common/print/logging.h"
#include "../common/signals/cleanup_signal.h"
#include "../common/threads/alive_socket_thread.h"
#include "./threads/ss_requests.h"

// #include "./threads/thread_for_client.h"

typedef struct StorageServer {
    char cwd[MAX_PATH_LEN]; /* Current working directory */
    int passiveSockfd;      /* Passive socket used by nm to communicate with SS   */
    int passiveSockPort;    /* Port for PassiveSockfd */
    AccessiblePaths paths;  /* List of paths accessible by SS */
    int nmSockfd;           /* Active socket for talking to NM */
    int clientSockfd;       /* Passive socket for client to connect to SS */
    int clientSockPort;     /* Port for clientSockfd */
    // ThreadForClient cltThread;

    Requests requests;
    AliveSocketThread aliveThread;
    /* Cleanup stuff */
    sig_atomic_t isCleaningup;
    ErrorCode exitCode;
} StorageServer;

extern StorageServer ss;

ErrorCode initSS();
ErrorCode inputPaths();
ErrorCode connectToNM();
void destroySS();

#endif