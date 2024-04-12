#ifndef __NETWORKING_H
#define __NETWORKING_H

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../error/error.h"

extern const char* GLOBAL_IP;        /* Local host 127.0.0.1 */
extern const int CLIENT_LISTEN_PORT; /* Port on which the NM listens for clients */
extern const int SS_LISTEN_PORT;     /* Port on which the NM listens for storage servers */
extern const int SS_REQ_LISTEN_PORT;     /* Port on which the NM listens for storage servers */

/*
    Passing port as 0 creates a passive socket on a random port
*/
ErrorCode createPassiveSocket(int* sockfd, int port);
ErrorCode createActiveSocket(int* sockfd);

ErrorCode connectToServer(int clientSockfd, int serverPort);
ErrorCode acceptClientNonBlocking(int serverSockfd, int* clientSockfd, bool *didConnect);

bool canConnectToServer(int clientSockfd, int serverPort);
/*
    Does not print error message in case of FAILURE
*/
ErrorCode acceptClient(int serverSockfd, int* clientSockfd);

/*
    Does not print error message in case of FAILURE
*/
ErrorCode socketSend(int sockfd, void* dataPtr, size_t bytes);

/*
    Does not print error message in case of FAILURE
*/
ErrorCode socketRecieve(int sockfd, void* dataPtr, size_t bytes);

/*
    Does not print error message in case of FAILURE, ERRORCODE
*/

ErrorCode socketRecieveTimeout(int sockfd, void* dataPtr, size_t bytes, int millis, bool* recieved);

/*
    Does not print error message in case of FAILURE
*/
ErrorCode closeSocket(int sockfd);

ErrorCode getPort(int sockfd, int* port);
#endif