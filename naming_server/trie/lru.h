#ifndef __LRU_H
#define __LRU_H

#include "../../common/error/error.h"
#include "../../common/networking/ack/ssinfo.h"

#define MAX_PATH_LEN 256

typedef struct CacheStruct {
    char path[MAX_PATH_LEN];
    SSInfo ssinfo;
    struct CacheStruct* next;
    struct CacheStruct* prev;
} CacheStruct;

typedef CacheStruct* Cache;

#define MAX_CACHE_SIZE 16

typedef struct LRU {
    int size;
    Cache front;
    Cache rear;
} LRU;

extern LRU lru;

void initLRU();
void destroyLRU();

SSInfo checkCache(char* path);
void removeSSFromLRU(SSInfo ssinfo);
void pushFront(char* path, SSInfo ssinfo);
SSInfo checkCache(char* path);
void deleteFromLRU(char* path);

#endif