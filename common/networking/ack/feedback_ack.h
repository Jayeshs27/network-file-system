#ifndef __FEEDBACK_ACK_H
#define __FEEDBACK_ACK_H

#include "../../error/error.h"

typedef struct FeedbackAck {
    ErrorCode errorCode;
} FeedbackAck;

#endif