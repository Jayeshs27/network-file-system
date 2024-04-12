#include "request_type.h"

const char* const REQ_TYPE_TO_STRING[] = {
    [REQUEST_CREATE] = "REQUEST_CREATE",
    [REQUEST_DELETE] = "REQUEST_DELETE",
    [REQUEST_COPY] = "REQUEST_COPY",
    [REQUEST_READ] = "REQUEST_READ",
    [REQUEST_WRITE] = "REQUEST_WRITE",
    [REQUEST_METADATA] = "REQUEST_METADATA",
    [REQUEST_LIST] = "REQUEST_LIST",
    [REQUEST_COPY_READ] = "REQUEST_COPY_READ",
    [REQUEST_COPY_WRITE] = "REQUEST_COPY_WRITE"
};

bool isPrivileged(RequestType type) {
    switch (type) {
        case REQUEST_DELETE:
        case REQUEST_CREATE:
        case REQUEST_COPY:
        case REQUEST_COPY_READ:
        case REQUEST_COPY_WRITE:
            return true;
        
        case REQUEST_READ:
        case REQUEST_WRITE:
        case REQUEST_METADATA:
        case REQUEST_LIST:
            return false;
    }

    return false;
}