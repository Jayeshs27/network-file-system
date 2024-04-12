#ifndef __ALIVE_SOCKET_THREAD_H
#define __ALIVE_SOCKET_THREAD_H

#include <pthread.h>
#include <stdbool.h>

#include "../error/error.h"

typedef struct AliveSocketThread {
    pthread_t thread;  // init this as 0
    int alivePort;
    int aliveSocket;
} AliveSocketThread;

ErrorCode initAliveSocketThread(AliveSocketThread* thread);
void startAliveSocketThread(AliveSocketThread* thread);

bool isCleaningUp();
void initiateCleanup(ErrorCode exitCode);

#endif