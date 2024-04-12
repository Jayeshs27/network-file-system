#include "cleanup_signal.h"

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "../print/logging.h"

jmp_buf envBuffer;
struct sigaction sa;
int jmpRet = 0;
void (*cleanupFunc)();

void passSignalNumber(int signalNumber) {
    longjmp(envBuffer, signalNumber);
}

/* cleanup must terminate the program */
void initEscapeHatch(void (*cleanup)()) {
    sa.sa_handler = passSignalNumber;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    cleanupFunc = cleanup;
    jmpRet = setjmp(envBuffer);
    sigaction(SIGINT, &sa, NULL);

    if (jmpRet) {
        lprintf("Recived Signal: %s", strsignal(jmpRet));
        lprintf("Cleaning up and exiting");
        (*cleanupFunc)();
    }
}

void destroyEscapeHatch() {
}
