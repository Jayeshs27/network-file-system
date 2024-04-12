#include "metadata.h"

ErrorCode sendMDRequest(MDRequest* request, int sockfd){
    return __sendRequest(request, sizeof(MDRequest), sockfd);
}
ErrorCode recieveMDRequest(MDRequest* request, int sockfd){
    return __recieveRequest(request, sizeof(MDRequest), sockfd);

}