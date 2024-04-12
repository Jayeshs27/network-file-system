#ifndef __CLIENT_ALIVE_THREAD
#define __CLIENT_ALIVE_THREAD

#include <pthread.h>

#include "../../../common/networking/nm_client/client_connect.h"
#include "../../naming_server.h"

ErrorCode createClientAliveThread(pthread_t* aliveThread);

#endif