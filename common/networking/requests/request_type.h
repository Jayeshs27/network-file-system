#ifndef __REQUEST_TYPE_H
#define __REQUEST_TYPE_H

#include <stdbool.h>

typedef enum RequestType {
    /* Privileged requests */
    REQUEST_CREATE,
    REQUEST_DELETE,
    REQUEST_COPY,
    /* Non privileged requests*/
    REQUEST_READ,
    REQUEST_WRITE,
    REQUEST_METADATA,
    REQUEST_LIST,
    REQUEST_COPY_READ,
    REQUEST_COPY_WRITE
} RequestType;

bool isPrivileged(RequestType type);

extern const char* const REQ_TYPE_TO_STRING[9];

#endif