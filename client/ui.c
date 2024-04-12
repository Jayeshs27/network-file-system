#include <stdio.h>
#include <pthread.h>
#include "../common/networking/requests/request_type.h"
#include "../common/networking/networking.h"

// Function that each thread will execute
void* clientHandler(void* arg) {
    int clientSocket = *((int*)arg);
    
  
    closeSocket(clientSocket);

    return NULL;
}

int main() {
    int port = 5555;
    int sockfd;
    createPassiveSocket(&sockfd, port);

    while (1) {
        
        int clientSocket;
        if (acceptClient(sockfd, &clientSocket) == 0) {
           
            pthread_t thread;
            pthread_create(&thread, NULL, clientHandler, (void*)&clientSocket);

            
            pthread_detach(thread);
        }
    }

    // Close the server socket (not reached in this example)
    closeSocket(sockfd);

    return 0;
}