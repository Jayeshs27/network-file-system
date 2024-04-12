#ifndef __ERROR_H
#define __ERROR_H

#include <string.h>
#include <errno.h>
#define FAILURE 1
#define SUCCESS 0

typedef int ErrorCode;

void eprintf(const char* format, ...);

#define EPATHNOTFOUND 10
#define ENO_SS 11
#define EPERM_DENIED 12 // PERMISSION DENIDE
#define EEMPTY_PATH 13   // EMPTY PATH
#define ENOT_DIRECT 14
#define EIVAL_PATH 15   // invalid path
#define EIVAL_DESCRIPTOR 16
#define EIVAL_FLAG  17
#define EALREADY_EXIST 18
#define EEBUSY  19       // busy (system preventing it's removal)
#define EIS_DIRECT 20 


ErrorCode StatErrors();
ErrorCode OpenDirErrors();
ErrorCode CloseDirErrors();
ErrorCode FileOpenErrors();
ErrorCode MKDirErrors();
ErrorCode rmErrors();
ErrorCode ReadErrors();

#endif