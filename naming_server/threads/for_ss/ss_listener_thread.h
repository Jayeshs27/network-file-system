#ifndef __SS_LISTENER_THREAD
#define __SS_LISTENER_THREAD

#include <pthread.h>

#include "../../../common/networking/nm_ss/ss_connect.h"
#include "../../naming_server.h"

ErrorCode createSSListenerThread(pthread_t* listenerThread);

#endif