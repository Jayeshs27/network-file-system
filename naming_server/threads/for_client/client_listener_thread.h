#ifndef __CLIENT_LISTENER_THREAD
#define __CLIENT_LISTENER_THREAD

#include <pthread.h>

#include "../../../common/networking/nm_client/client_connect.h"
#include "../../naming_server.h"

ErrorCode createClientListenerThread(pthread_t* listenerThread);

#endif