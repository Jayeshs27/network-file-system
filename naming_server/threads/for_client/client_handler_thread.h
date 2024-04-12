#ifndef __CLIENT_HANDLER_THREAD_H
#define __CLIENT_HANDLER_THREAD_H

#include <pthread.h>
#include <stdbool.h>

#include "../../naming_server.h"

void startClientThread(ConnectedClient client);
void killClientThread(ConnectedClient client);
bool isModified(RequestType type);
void ModifySSCopy(SSInfo ssinfo,RequestType type,void* request);

#endif