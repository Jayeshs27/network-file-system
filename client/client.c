#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/networking/networking.h"

#define JOIN_IF_CREATED(T, RET)      \
    do {                             \
        if (T) pthread_join(T, RET); \
    } while (0)

void signalSuccess();

Client client;

ErrorCode initClient() {
    client.isCleaningup = 0;
    client.exitCode = SUCCESS;
    initEscapeHatch(signalSuccess);
    if (initLogger("logs/client/", false)) {
        eprintf("Could not create log file\n");
        return FAILURE;
    }

    if (startLogging()) {
        eprintf("Could not start logging\n");
        goto destroy_logger;
    }

    lprintf("Main : Creating Alive Thread for Client's Alive Socket");
    if (initAliveSocketThread(&client.thread)) {
        goto destroy_logging;
    }

    lprintf("Main : Creating Passive Socket for Client's Passive Socket");
    if (createPassiveSocket(&client.passiveSockfd, 0)) {
        goto destroy_aliveSocketthread;
    }

    lprintf("Main : Getting port for Client's Passive Socket");
    if (getPort(client.passiveSockfd, &client.passiveSockPort)) {
        goto destroy_passivesockfd;
    }

    lprintf("Main : Creating Active Socket for Client's NM Socket");
    if (createActiveSocket(&client.nmSockfd)) {
        goto destroy_passivesockfd;
    }

    return SUCCESS;

    // destroy_nmsockfd:
    //     close(client.nmSockfd);

destroy_passivesockfd:
    close(client.passiveSockfd);

destroy_aliveSocketthread:
    close(client.thread.aliveSocket);

destroy_logging:
    destroyEscapeHatch();
    endLogging();

destroy_logger:
    destroyLogger();

    return FAILURE;
}

bool isCleaningUp() {
    return client.isCleaningup;
}

void initiateCleanup(ErrorCode exitCode) {
    client.isCleaningup = 1;
    client.exitCode = exitCode;
}

void destroyClient() {
    lprintf("Main : Closing all sockfds in Client");

    shutdown(client.nmSockfd, SHUT_RDWR);

    shutdown(client.thread.aliveSocket, SHUT_RDWR);
    JOIN_IF_CREATED(client.thread.thread, NULL);

    lprintf("Main : Alive_socket joined");
    closeSocket(client.passiveSockfd);

    endLogging();
    destroyLogger();
    JOIN_IF_CREATED(getLoggingThread(), NULL);
    exit(client.exitCode);
}

ErrorCode connectToNM() {
    if (connectToServer(client.nmSockfd, CLIENT_LISTEN_PORT)) {
        client.exitCode = FAILURE;
        return FAILURE;
    }

    ClientInitRequest req;
    req.clientAlivePort = client.thread.alivePort;
    req.clientPassivePort = client.passiveSockPort;

    if (sendClientRequest(client.nmSockfd, &req)) {
        client.exitCode = FAILURE;
        return FAILURE;
    }

    return SUCCESS;
}

void* inputRequest(RequestType* requestType) {
    int option;
    printf("Select an option:\n");
    printf("1. Write to File\n");
    printf("2. Read a file\n");
    printf("3. Delete a file/folder\n");
    printf("4. Create a file\n");
    printf("5. Create a folder\n");
    printf("6. List a folder\n");
    printf("7. Get metadata\n");
    printf("8. Copy folder\n");
    scanf("%d", &option);
    void* ret = NULL;
    switch (option) {
        case 1:
            *requestType = REQUEST_WRITE;
            ret = calloc(1, sizeof(WriteRequest));
            printf("Enter file path: ");
            scanf("%s", ((WriteRequest*)ret)->path);
            printf("Enter content to write: ");
            scanf("%s", ((WriteRequest*)ret)->Data);
            break;
        case 2:
            *requestType = REQUEST_READ;
            ret = calloc(1, sizeof(ReadRequest));
            printf("Enter file path: ");
            scanf("%s", ((ReadRequest*)ret)->path);
            break;
        case 3:
            *requestType = REQUEST_DELETE;
            ret = calloc(1, sizeof(DeleteRequest));
            printf("Enter file path to delete: ");
            scanf("%s", ((DeleteRequest*)ret)->path);
            break;
        case 4:
            *requestType = REQUEST_CREATE;
            ret = calloc(1, sizeof(CreateRequest));
            printf("Enter file path: ");
            scanf("%s", ((CreateRequest*)ret)->path);
            printf("Enter file name: ");
            scanf("%s", ((CreateRequest*)ret)->newObject);
            ((CreateRequest*)ret)->isDirectory = false;
            break;
        case 5:
            *requestType = REQUEST_CREATE;
            ret = calloc(1, sizeof(CreateRequest));
            printf("Enter folder path: ");
            scanf("%s", ((CreateRequest*)ret)->path);
            ((CreateRequest*)ret)->isDirectory = true;
            break;
        case 6:
            *requestType = REQUEST_LIST;
            ret = calloc(1, sizeof(ListRequest));
            printf("Enter folder path to list: ");
            scanf("%s", ((ListRequest*)ret)->path);
            break;
        case 7:
            *requestType = REQUEST_METADATA;
            ret = calloc(1, sizeof(MDRequest));
            printf("Enter file/folder path to get size:");
            scanf("%s", ((MDRequest*)ret)->path);
            break;
        case 8:
            *requestType = REQUEST_COPY;
            ret = calloc(1, sizeof(CopyRequest));
            printf("Enter source path: ");
            scanf("%s", ((CopyRequest*)ret)->SrcPath);
            printf("Enter destination path: ");
            scanf("%s", ((CopyRequest*)ret)->DestPath);
            break;
        default:
            printf("Invalid option\n");
            return NULL;
    }

    return ret;
}

ErrorCode inputAndSendRequest() {
    ErrorCode ret = SUCCESS;
    RequestType type;
    void* request = NULL;
    while (request == NULL) request = inputRequest(&type);
    lprintf("Main : sending RequestType..");
    if ((ret = sendRequestType(&type, client.nmSockfd))) {
        eprintf("Could not send request type\n");
        goto destroy_request;
    }

    bool recievedAck;
    RequestTypeAck requestTypeAck;
    recieveRequestTypeAck(&requestTypeAck, client.nmSockfd, TIMEOUT_MILLIS, &recievedAck);

    if (!recievedAck) {
        eprintf("RequestTypeAck timed out\n");
        ret = FAILURE;
        goto destroy_request;
    }
    lprintf("Main : received RequestType ack");
    if ((ret = sendRequest(type, request, client.nmSockfd))) {
        eprintf("Could not send request\n");
        goto destroy_request;
    }
    lprintf("Main : request sent");

    if (type == REQUEST_LIST) {
        ListResponse response;
        if (socketRecieve(client.nmSockfd, &response, sizeof(ListResponse))) {
            eprintf("Could not recieve list response\n");
            ret = FAILURE;
            goto destroy_request;
        }

        FeedbackAck ack;
        if (recieveFeedbackAck(&ack, client.nmSockfd)) {
            eprintf("Could not Receive feedbackAck from SS\b");
            ret = FAILURE;
            goto destroy_request;
        }

        if (ack.errorCode == EPATHNOTFOUND) {
            eprintf("Path does not exist in NM\n");
            goto destroy_request;
        }

        if(ack.errorCode == SUCCESS)
        {
            cprintf(GREEN_BOLD,"%s Executed successfully\n",REQ_TYPE_TO_STRING[type]);
        }
        else{
            eprintf("Error While Executing,Errorcode = %d\n",ack.errorCode);
            goto destroy_request;
            
        }

        printf("%d Children\n", response.list_cnt);
        for(int i = 0; i < response.list_cnt; i++) {
            printf("\t%s\n", response.list[i]);
        }

        goto destroy_request;
    }

    if (isPrivileged(type)) {
        FeedbackAck fdAck;
        if (recieveFeedbackAck(&fdAck, client.nmSockfd)) {
            eprintf("Could not Receive feedbackAck from SS\b");
            ret = FAILURE;
            goto destroy_request;
        }
        if (fdAck.errorCode == EPATHNOTFOUND) {
            eprintf("Path does not exist in NM\n");
            goto destroy_request;
        }
        else if(fdAck.errorCode == SUCCESS)
        {
            cprintf(GREEN_BOLD,"%s Executed successfully\n",REQ_TYPE_TO_STRING[type]);
        }
        else{
            eprintf("Error While Executing,Errorcode = %d\n",fdAck.errorCode);
            ret = fdAck.errorCode;
            goto destroy_request;
        }
    } else {
        SSInfo ssinfo;
        if (recieveSSInfo(&ssinfo, client.nmSockfd)) {
            eprintf("Could not recieve ssinfo\n");
            ret = FAILURE;
            goto destroy_request;
        }
        lprintf("Main : recieved ssinfo ssClientPort = %d, ssPassivePort = %d", ssinfo.ssClientPort, ssinfo.ssPassivePort);
        if (ssinfo.ssClientPort == -1) {
            eprintf("Path does not exist in NM\n");
            goto destroy_request;
        }

        int sockfd;
        if (createActiveSocket(&sockfd)) {
            ret = FAILURE;
            goto destroy_request;
        }
        lprintf("Main : Active Socket Created");
        if (connectToServer(sockfd, ssinfo.ssClientPort)) {
            ret = FAILURE;
            goto destroy_request;
        }
        lprintf("Main : Connected to SS");
        if ((ret = sendRequestType(&type, sockfd))) {
            goto destroy_request;
        }
        lprintf("Main : RequestType sent to SS");
        bool RecAck;
        RequestTypeAck typeAck;
        recieveRequestTypeAck(&typeAck, sockfd, TIMEOUT_MILLIS, &RecAck);

        if (!RecAck) {
            eprintf("RequestTypeAck timed out\n");
            ret = FAILURE;
            goto destroy_request;
        }
        lprintf("Main : RequestType Ack received");

        if ((ret = sendRequest(type, request, sockfd))) {
            eprintf("Could not send Request to SS\n");
            goto destroy_request;
        }
        lprintf("Main : Request sent");
        switch (type) {
            case REQUEST_WRITE:
                break;
            case REQUEST_READ:
                if ((ret = ReadResponseHandler(sockfd))) {
                    goto destroy_request;
                }
                break;
            case REQUEST_METADATA:
                if ((ret = MetaDataResponseHandler(sockfd))) {
                    goto destroy_request;
                }
                break;
            case REQUEST_LIST:
                if ((ret = ListResponseHandler(sockfd))) {
                    goto destroy_request;
                }
                break;
            default:
                break;
        }
        lprintf("Main : Waiting for feedback ack");
        FeedbackAck fdAck;
        if (recieveFeedbackAck(&fdAck, sockfd)) {
            eprintf("Could not receive Write feedbackAck\n");
            ret = FAILURE;
            goto destroy_request;
        }
        lprintf("Main : FeedbackAck Received for %s : %d", REQ_TYPE_TO_STRING[type], fdAck);
        if (fdAck.errorCode == EPATHNOTFOUND) {
            eprintf("Path does not exist in NM\n");
        }
        else if(fdAck.errorCode == SUCCESS)
        {
            cprintf(GREEN_BOLD,"%s Executed successfully\n",REQ_TYPE_TO_STRING[type]);
        }
        else{
            eprintf("Error While Executing,Errorcode = %d\n",fdAck.errorCode);
            ret = fdAck.errorCode;
        }
        if(sendFeedbackAck(&fdAck,client.nmSockfd)){
            eprintf("Could not send feedbackAck to NM\n");
            goto destroy_request;
        }
        lprintf("Main : feedbackAck for %s sent to NM",REQ_TYPE_TO_STRING[type]);
    }

destroy_request:
    destroyRequest(request);
    return ret;
}

void destroyRequest(void* request) {
    free(request);
}

void signalSuccess() {
    client.isCleaningup = 1;
    destroyClient();
}