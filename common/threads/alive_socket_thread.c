#include "alive_socket_thread.h"

#include <errno.h>
#include <signal.h>
#include <errno.h>

#include "../networking/networking.h"
#include "../print/logging.h"

void cleanAliveSocketThread(AliveSocketThread* thread) {
    close(thread->aliveSocket);
}

ErrorCode initAliveSocketThread(AliveSocketThread* thread) {
    if (createPassiveSocket(&thread->aliveSocket, 0)) {
        return FAILURE;
    }
    getPort(thread->aliveSocket, &thread->alivePort);
    thread->thread = 0;
    return SUCCESS;
}

void* aliveSocketThreadRoutine(void* arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    AliveSocketThread* thread = (AliveSocketThread*)arg;
    while (!isCleaningUp()) {
        int clientfd;
        // lprintf("Alive_socket : Accpeting");

        if (acceptClient(thread->aliveSocket, &clientfd)) {
            initiateCleanup(FAILURE);
            eprintf("Could not acceptClient(), errno = %d %s\n", errno, strerror(errno));
            break;
        }

        // lprintf("Alive_socket : Closing");
        close(clientfd);
    }
    lprintf("Alive Thread : Cleaning up");
    cleanAliveSocketThread(thread);
    return NULL;
}

void startAliveSocketThread(AliveSocketThread* thread) {
    pthread_create(&thread->thread, NULL, aliveSocketThreadRoutine, thread);
}
