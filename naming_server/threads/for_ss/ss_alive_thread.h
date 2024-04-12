#ifndef __SS_ALIVE_THREAD
#define __SS_ALIVE_THREAD

#include <pthread.h>

#include "../../../common/networking/nm_client/client_connect.h"
#include "../../naming_server.h"

ErrorCode createSSAliveThread(pthread_t* aliveThread);

#endif