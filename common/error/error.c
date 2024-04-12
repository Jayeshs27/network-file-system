#include "error.h"

#include <stdarg.h>

#include "../print/print.h"

void eprintf(const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    cfprintf(stderr, RED_BOLD, "[ ERROR ] ");
    cvfprintf(stderr, RED_BOLD, format, arguments);
}

ErrorCode StatErrors(){
    if(errno == EFAULT){
        return EIVAL_PATH;
    }
    if(errno == EACCES){
        return EPERM_DENIED;
    }
    if(errno == ENOENT){
        return EEMPTY_PATH;
    }
    return FAILURE;
}


ErrorCode OpenDirErrors(){
    if(errno == EACCES){
        return EPERM_DENIED;
    }
    if(errno == ENOTDIR){
        return ENOT_DIRECT;
    }
    return FAILURE;
}

ErrorCode CloseDirErrors(){
    if(errno == EBADF){
        return EIVAL_DESCRIPTOR;
    }
    return FAILURE;
}

ErrorCode FileOpenErrors(){
    if(errno == EINVAL){
        return EIVAL_FLAG;
    }
    if(errno == EEXIST){
        return EALREADY_EXIST;
    }
    return FAILURE;
}

ErrorCode MKDirErrors(){
    if(errno == EEXIST){
        return EALREADY_EXIST;
    }
    if(errno == EINVAL){
        return EPERM_DENIED;
    }
    return FAILURE;
}
ErrorCode rmErrors(){
    if(errno == EFAULT){
        return EPATHNOTFOUND;
    }
    if(errno == EBUSY){
         return EEBUSY;
    }
    return FAILURE;
}
ErrorCode ReadErrors(){
    if(EISDIR == errno){
         return EIS_DIRECT;
    }
    if(EBADF == errno){
         return EIVAL_DESCRIPTOR;
    }
    return FAILURE;
}