#ifndef __CLIENT_HANDLER_H
#define __CLIENT_HANDLER_H

#include <sys/socket.h>

#include "../../../common/networking/requests.h"
#include "../../naming_server.h"


void ExtractPath(void* request, RequestType reqType, char* Path);

ErrorCode Copyhandler(void* request);
ErrorCode handleListRequest(void* rawRequest, ConnectedClient client);
ErrorCode handleNP(RequestType type, void* rawRequest, ConnectedClient client);
ErrorCode handleCreateRequest(void* rawRequest, ConnectedClient client);
ErrorCode handleDeleteRequest(void* rawRequest, ConnectedClient client);
#endif