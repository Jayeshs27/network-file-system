#include "naming_server.h"

#include <unistd.h>

int main() {
    if (initNM()) {
        destroyNM();
    }

    createSSListenerThread(&namingServer.ssListener);
    createSSAliveThread(&namingServer.ssAliveChecker);
    createClientListenerThread(&namingServer.clientListener);
    createClientAliveThread(&namingServer.clientAliveChecker);

    while (!isCleaningUp()) {
        // usleep(500000);
    }
    // pthread_join(namingServer.ssListener, NULL);
    // pthread_join(namingServer.ssAliveChecker, NULL);
    // pthread_join(namingServer.clientListener, NULL);
    // pthread_join(namingServer.clientAliveChecker, NULL);
    destroyNM();
}