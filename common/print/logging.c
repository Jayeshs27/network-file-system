#include "logging.h"

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#define UNUSED(x) (void)x

#define MAX_LOG_FILE_NAME 64

#define MAX_LOG_TEXT 512
struct LoggerStruct {
    pthread_mutex_t lock;
    char buffer[MAX_LOG_TEXT];
    pthread_t loggingThread;
    bool silent;
    FILE* logFile;
    pthread_cond_t isFull;
    pthread_cond_t isEmpty;
    bool isCleaningup;
    bool size;
} Logger;

ErrorCode initLogger(char* logDirectory, bool silent) {
    time_t timer = time(NULL);
    struct tm* timeNow = localtime(&timer);
    char* fullPath = malloc(strlen(logDirectory) + MAX_LOG_FILE_NAME + 2);
    strcpy(fullPath, logDirectory);
    char fileName[MAX_LOG_FILE_NAME];
    strftime(fileName, MAX_LOG_FILE_NAME, "%y_%m_%d-%T.log", timeNow);
    strcat(fullPath, fileName);

    Logger.logFile = fopen(fullPath, "w");
    if (Logger.logFile == NULL) {
        Logger.logFile = fopen(fileName, "w");
        strcpy(fullPath, fileName);
        if (Logger.logFile == NULL) {
            free(fullPath);
            return FAILURE;
        }
    }
    if (!silent) {
        fprintf(stderr, "Logging to %s\n", fullPath);
    }
    free(fullPath);

    pthread_mutex_init(&Logger.lock, NULL);
    pthread_cond_init(&Logger.isEmpty, NULL);
    pthread_cond_init(&Logger.isFull, NULL);
    Logger.loggingThread = 0;
    Logger.size = 0;
    Logger.silent = silent;
    Logger.isCleaningup = false;
    return SUCCESS;
}

void destroyLogger() {
    if (Logger.logFile) fclose(Logger.logFile);
    pthread_mutex_destroy(&Logger.lock);
    pthread_cond_destroy(&Logger.isFull);
    pthread_cond_destroy(&Logger.isEmpty);
}

#define MAX_TIME_STAMP 64
#define TIME_STAMP_FORMAT "[ %F %T ]"

void lprintf(char* format, ...) {
    char timestamp[MAX_TIME_STAMP];
    time_t timer = time(NULL);
    va_list arguments;
    va_start(arguments, format);
    pthread_mutex_lock(&Logger.lock);
    while (Logger.size != 0) {
        pthread_cond_wait(&Logger.isEmpty, &Logger.lock);
    }

    struct tm* timeNow = localtime(&timer);
    strftime(timestamp, MAX_TIME_STAMP, TIME_STAMP_FORMAT, timeNow);
    strcpy(Logger.buffer, timestamp);
    char logMessage[MAX_LOG_TEXT];
    vsprintf(logMessage, format, arguments);
    sprintf(Logger.buffer, "%s %s", timestamp, logMessage);
    Logger.size = 1;
    pthread_cond_signal(&Logger.isFull);
    pthread_mutex_unlock(&Logger.lock);
}

void* logRoutine(void* arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    UNUSED(arg);

    while (true) {
        // printf("Logger locking lock\n");
        pthread_mutex_lock(&Logger.lock);
        // printf("Logger got lock\n");

        while (Logger.size != 1) {
            pthread_cond_wait(&Logger.isFull, &Logger.lock);
        }

        if (Logger.isCleaningup) break;

        fprintf(Logger.logFile, "%s\n", Logger.buffer);
        if (!Logger.silent) fprintf(stderr, "%s\n", Logger.buffer);
        fflush(Logger.logFile);
        Logger.size = 0;
        pthread_cond_broadcast(&Logger.isEmpty);
        pthread_mutex_unlock(&Logger.lock);
    }

    printf("Cleaning up logger\n");
    return NULL;
}

ErrorCode startLogging() {
    if (pthread_create(&Logger.loggingThread, NULL, logRoutine, NULL)) {
        return FAILURE;
    }

    return SUCCESS;
}

void endLogging() {
    if (Logger.loggingThread == 0) return;
    pthread_mutex_lock(&Logger.lock);
    while (Logger.size != 0) {
        pthread_cond_wait(&Logger.isEmpty, &Logger.lock);
    }
    Logger.isCleaningup = true;
    Logger.size = 1;
    pthread_mutex_unlock(&Logger.lock);
    pthread_cond_signal(&Logger.isFull);
}

pthread_t getLoggingThread() {
    return Logger.loggingThread;
}