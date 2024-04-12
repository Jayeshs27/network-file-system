#include "client_requests.h"

#include <stdio.h>
#include <time.h>

void printFileInfo(struct stat *fileStat) {
    printf("Size: %ld bytes\n", fileStat->st_size);
    printf("Permissions: ");
    printf((S_ISDIR(fileStat->st_mode)) ? "d" : "-");
    printf((fileStat->st_mode & S_IRUSR) ? "r" : "-");
    printf((fileStat->st_mode & S_IWUSR) ? "w" : "-");
    printf((fileStat->st_mode & S_IXUSR) ? "x" : "-");
    printf((fileStat->st_mode & S_IRGRP) ? "r" : "-");
    printf((fileStat->st_mode & S_IWGRP) ? "w" : "-");
    printf((fileStat->st_mode & S_IXGRP) ? "x" : "-");
    printf((fileStat->st_mode & S_IROTH) ? "r" : "-");
    printf((fileStat->st_mode & S_IWOTH) ? "w" : "-");
    printf((fileStat->st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");

    printf("Last Access Time: %s\n", ctime(&fileStat->st_atime));
    // printf("Last Modification Time: %s\n", ctime(&fileStat->st_mtime));
    // printf("Last Status Change Time: %s\n", ctime(&fileStat->st_ctime));

    // Add any additional information you want to print
}

ErrorCode ReadResponseHandler(int sockfd) {
    ReadPacket packet;
    memset(packet.data, '\0', MAX_DATA_LENGTH);
    if (receivePacket(&packet, sockfd)) {
        return FAILURE;
    }
    
    while (packet.header != STOP_PKT) {
        printf("%s", packet.data);
        memset(packet.data, '\0', MAX_DATA_LENGTH);
        if (receivePacket(&packet, sockfd))
            return FAILURE;
    }

    printf("\n");

    return SUCCESS;
}

ErrorCode MetaDataResponseHandler(int sockfd) {
    struct stat st;
    if (socketRecieve(sockfd, &st, sizeof(struct stat))) {
        eprintf("Could not receive metadata\n");
        return FAILURE;
    }
    lprintf("Main : MetaData Received");
    printFileInfo(&st);
    return SUCCESS;
}

ErrorCode ListResponseHandler(int sockfd) {
    ListResponse Response;
    if (socketRecieve(sockfd, &Response, sizeof(ListResponse))) {
        eprintf("Could not receive list\n");
        return FAILURE;
    }
    
    lprintf("Main : List Received");

    for (int i = 0; i < Response.list_cnt; ++i) {
        printf("%s\n", Response.list[i]);
    }
    return SUCCESS;
}