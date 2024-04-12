#include <poll.h>

#include "client.h"
#include "../common/networking/requests.h"

int main() {
    if (initClient()) {
        destroyClient();
    }
    startAliveSocketThread(&client.thread);
    if (connectToNM()) {
        destroyClient();
    }

    while (!isCleaningUp()) {
        inputAndSendRequest();
    }
    while (!isCleaningUp()) {
        // printf("is cleaing up : ");
    }
    destroyClient();
}

/*
  (Client Side)
    1. do not close nmsockfd
    2. operation requests will be sent on nmsockfd
    3. operation requests sent in 2 steps
        - send type of request
        - listens for request_type_ack (timeout)
        - send actual request
        - (block and wait for message from nm)
        - if read/write/perms/size
            - recieve address of ss.
            - connect to this port
            - send request type
            - send actual request
            - block and wait for ss
            - recieve feedback from ss

        - else if create/delete/copy:
            - block till ack from nm
            - recieve feedback ack containing success/failure errorcode from nm.

    (NM side)
    1. when clients inits, create a dedicated thread for that client
    2. on that thread, listen for request type from client
    3. send request_type_ack to that client
    4. recieve actual request from client
    5. if read/write/metadata search the trie for address of ss, and send it to client
    6. if create/delete/copy 
        - find the addresses of required storage server(s)
        - connect to the passive port(s) of the storage servers(s), open a new active sockfd
        - add the request and sockfd to a list
        
    (SS Side)
        - listen for connections on passive port
        - create a new thread for each connection (request)
            - in this thread, recieve request type
            - recieve request
            - execute request
            - send feedback ack to connecter (could be nm or ss)

        - iterate throught the list
        - listen for feedbackcks
        - send actual request from storageServerSockfds[]
        - block that thread and wait for feedback from ss
        - disconnect from ss
        - forward this feedback_ack back to client

    (SS side)
    1. after init, create threads for listening to client and nm seperately

    (On client thread)
    1. wait for client to connect
    2. upon connection, recieve type of request (type can only be read/write/perms/size)
    5. recieve actual request
    6. execute request
    7. send feedback_ack to client
    8. client should disconnect

    (On NM thread)
    1. wait for nm to connect (on passivePort of ss)
    2. recieve type of request from nm
    3. recieve actual request from nm
    4. execute request
    5. send feedback_ack to nm
    6. nm should disconnect
*/