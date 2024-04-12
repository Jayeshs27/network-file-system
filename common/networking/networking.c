#include "networking.h"

#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>

const char* GLOBAL_IP = "127.0.0.1";
const int SS_REQ_LISTEN_PORT = 13301;
const int CLIENT_LISTEN_PORT = 7331;
const int SS_LISTEN_PORT = 10181;

#define MAX_CONNECTION_IN_QUEUE 10

ErrorCode createPassiveSocket(int* sockfd, int port) {
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        eprintf("Could not create passive socket, errno = %d, %s\n", errno, strerror(errno));
        return FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(GLOBAL_IP);

    if (bind(*sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        eprintf("Could not bind to port %d, errno = %d, %s\n", port, errno, strerror(errno));
        return FAILURE;
    }

    if (listen(*sockfd, MAX_CONNECTION_IN_QUEUE) == -1) {
        eprintf("Could not listen() on port %d, errno = %d, %s\n", port, errno, strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

ErrorCode createActiveSocket(int* sockfd) {
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        eprintf("Could not create active socket, errno = %d, %s\n", errno, strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

ErrorCode connectToServer(int clientSockfd, int serverPort) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = serverPort;
    serverAddr.sin_addr.s_addr = inet_addr(GLOBAL_IP);

    if (connect(clientSockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        eprintf("Could not connect to port %d, errno = %d, %s\n", serverPort, errno, strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

bool canConnectToServer(int clientSockfd, int serverPort) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = serverPort;
    serverAddr.sin_addr.s_addr = inet_addr(GLOBAL_IP);

    if (connect(clientSockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        return false;
    }

    return true;
}

ErrorCode acceptClient(int serverSockfd, int* clientSockfd) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen;
    clientAddrLen = sizeof(clientAddr);
    *clientSockfd = accept(serverSockfd, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (*clientSockfd == -1) {
        return FAILURE;
    }

    return SUCCESS;
}

ErrorCode acceptClientNonBlocking(int serverSockfd, int* clientSockfd, bool* didConnect) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen;
    clientAddrLen = sizeof(clientAddr);
    *clientSockfd = accept(serverSockfd, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (*clientSockfd == -1) {
        if (errno == EAGAIN) { // also check errno == EWOULDBLOCK in general
            *didConnect = false;
        } else {
            return FAILURE;
        }
    } else {
        *didConnect = true;
    }

    return SUCCESS;
}

ErrorCode socketSend(int sockfd, void* dataPtr, size_t bytes) {
    if (send(sockfd, dataPtr, bytes, 0) == -1) {
        return FAILURE;
    }

    return SUCCESS;
}

ErrorCode socketRecieve(int sockfd, void* dataPtr, size_t bytes) {
    ssize_t bytesRead = -1;

    if ((bytesRead = recv(sockfd, dataPtr, bytes, 0)) == -1) {
        return FAILURE;
    } else if (bytesRead == 0) {
        return FAILURE;
    }

    return SUCCESS;
}

ErrorCode socketRecieveTimeout(int sockfd, void* dataPtr, size_t bytes, int millis, bool* recieved) {
    *recieved = false;
    struct pollfd fd;
    fd.fd = sockfd;
    fd.events = POLLIN;
    int ret = poll(&fd, 1, millis);
    switch (ret) {
        case -1:
            return FAILURE;
        case 0:
            break;
        default:
            recv(sockfd, dataPtr, bytes, 0);
            *recieved = true;
            break;
    }

    return SUCCESS;
}

ErrorCode closeSocket(int sockfd) {
    if (close(sockfd) == -1) {
        return FAILURE;
    }

    return SUCCESS;
}

ErrorCode getPort(int sockfd, int* port) {
    struct sockaddr_in socketAddr;
    socklen_t socketAddrLen = sizeof(socketAddr);
    if (getsockname(sockfd, (struct sockaddr*)&socketAddr, &socketAddrLen) == -1) {
        eprintf("Could not getsockname() on sockfd = %d, errno = %d, %s\n", sockfd, errno, strerror(errno));
        return FAILURE;
    }

    *port = socketAddr.sin_port;

    return SUCCESS;
}