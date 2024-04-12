#ifndef __LOGGING_H
#define __LOGGING_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "../error/error.h"

/*
    Must logDirectory must end in '/'
*/
ErrorCode initLogger(char* logDirectory, bool silent);
ErrorCode startLogging();
pthread_t getLoggingThread();
pthread_cond_t* getLoggerFull();
void cleanUpLogger();
void lprintf(char* format, ...);

/*
    Blocking call
*/
void endLogging();
void destroyLogger();

#endif