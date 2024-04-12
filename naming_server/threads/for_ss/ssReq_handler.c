#include "ssReq_handler.h"
#include "../../trie/trie.h"
#include "../../../common/print/logging.h"
#include "../../naming_server.h"

ErrorCode ssCopy_handler(ssRequest req,SSInfo ssSrc,SSInfo ssDst){
    SSCopyRequest *copyReq = (SSCopyRequest*)req->request; // IT IS COPY WRITE REQUEST

    int sockSrc,sockDst;

    if (createActiveSocket(&sockSrc)) {
        eprintf("Could not Create Active Socket(for Src)");
        return FAILURE;
    }
    // lprintf("CopyHandler: Created active socket for src");
   
    if (connectToServer(sockSrc, ssSrc.ssClientPort)) {
        eprintf("Could not connect to Src SS");
        return FAILURE;
    }

    SSCopyRequest readReq, writeReq;
    strcpy(readReq.path, copyReq->otherpath);
    readReq.reqType = REQUEST_COPY_READ;
    // lprintf("CopyHandler: Connected to src");
    sendRequestType(&readReq.reqType, sockSrc);
    // lprintf("CopyHandler: Sent Request type to src");
    RequestTypeAck srcAck;
    bool srcRecvd;
    recieveRequestTypeAck(&srcAck, sockSrc, 30000, &srcRecvd);
    // lprintf("CopyHandler: Recieved req type ack from src");

    sendRequest(readReq.reqType, &readReq, sockSrc);
    // lprintf("CopyHandler: Sent Request to src");

    if (createActiveSocket(&sockDst)) {
        eprintf("Could not Create Active Socket(for Dst)");
        return FAILURE;
    }
    // lprintf("CopyHandler: Created active socket for dest");

    if (connectToServer(sockDst, ssDst.ssClientPort)) {
        eprintf("Could not connect to Dst SS");
        return FAILURE;
    }

    // lprintf("CopyHandler: Connected to dest");

    //  CODE WITHOUT ERROR HANDING  /////////
    strcpy(writeReq.path, ".copy/");
    strcat(writeReq.path, copyReq->path);
    writeReq.reqType = REQUEST_COPY_WRITE;
    writeReq.IsCopy = true;

    sendRequestType(&writeReq.reqType, sockDst);
    // lprintf("CopyHandler: Sent Request type to dest");

    RequestTypeAck destAck;
    bool destRecvd;
    recieveRequestTypeAck(&destAck, sockDst, 30000, &destRecvd);
    // lprintf("CopyHandler: Recieved req type ack from dest");

    sendRequest(writeReq.reqType, &writeReq, sockDst);
    // lprintf("CopyHandler: Sent Request to dest");

    CopyPacket packet;
    ReceivePacket(sockSrc, &packet);
    lprintf("CopyHandler: Recieved packet");
    while (packet.header != END) {
        SendPacket(sockDst, &packet);
        lprintf("CopyHandler: Send packet");
        if(packet.header == STOP){
            FeedbackAck Ack;
            recieveFeedbackAck(&Ack,sockDst);
            sendFeedbackAck(&Ack,sockSrc);
        }
        ReceivePacket(sockSrc, &packet);
        lprintf("CopyHandler: Recieved packet");
    }

    // lprintf("CopyHandler: Routing finished");

    FeedbackAck SrcAck, DstAck;
    recieveFeedbackAck(&SrcAck, sockSrc);
    recieveFeedbackAck(&DstAck, sockDst);
    close(sockSrc);
    close(sockDst);
    return SUCCESS;
}

// void setIsCopy(RequestType type, void* request){
//     switch(type){
//         case REQUEST_CREATE:
//              CreateRequest* Creq = (CreateRequest*)request;
//              Creq->IsCopy = true;
//              request = (void*)Creq;
//              break;
//         case REQUEST_DELETE:
//              DeleteRequest* Dreq = (DeleteRequest*)request;
//              Dreq->IsCopy = true;
//              request = (void*)Dreq;
//              break;
//         case REQUEST_WRITE:
//              WriteRequest* Wreq = (WriteRequest*)request;
//              Wreq->IsCopy = true;
//              request = (void*)Wreq;
//         default:
//     }
// }
bool IsCleaning(ssRequest request) {
    pthread_mutex_lock(&request->cleanupLock);
    bool ret = request->isCleaningup;
    pthread_mutex_unlock(&request->cleanupLock);
    return ret;
}

void* SSrequestRoutine(void* arg){
      ssRequest requestThread = (ssRequest)arg;
      RequestType type = requestThread->type;
      void* request = requestThread->request;
      SSInfo ssCopyinfo[2];
      ssCopyinfo[0] = requestThread->ssCopyInfo[0];
      ssCopyinfo[1] = requestThread->ssCopyInfo[1];
      lprintf("SSRR : Redudant modification started for %d",REQ_TYPE_TO_STRING[type]);
      if(!isCleaningUp() && !IsCleaning(requestThread)) {
         for(int i = 0 ; i < 2 ; i++){
             // IF REQUEST != COPY
             if(type == REQUEST_COPY_WRITE){
                 int index = getSSIndexFromCwd(((SSCopyRequest*)request)->otherpath);
                 if(index != -1)
                    ssCopy_handler(requestThread,namingServer.map.ssinfos[index],requestThread->ssCopyInfo[i]);
             }
             else{
                int sockfd;
                if(createActiveSocket(&sockfd)){
                    // error;
                }
                lprintf("SSRR : active socket created");
                if(connectToServer(sockfd, ssCopyinfo[i].ssClientPort)) {
                    // error
                }
                lprintf("SSRR : connected to ssCopy %d",i+1);
                if(sendRequestType(&type, sockfd)){
                    // e
                }
                // setIsCopy(type,request);
                lprintf("SSRR : requestType sent");
                if(sendRequest(type, request, sockfd)){
                    // 
                }
                lprintf("SSRR : request sent");
                FeedbackAck ack;
                if(recieveFeedbackAck(&ack, sockfd)){

                }
                lprintf("SSRR : feedbackAck received : %d",ack.errorCode);
                close(sockfd);
             }
         }
      }

    return NULL;
}

