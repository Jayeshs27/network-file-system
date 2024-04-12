#include "lru.h"

#include <assert.h>
#include <stdlib.h>

LRU lru;

Cache newCache(char* path, SSInfo ssinfo) {
    Cache ret = calloc(1, sizeof(CacheStruct));
    strcpy(ret->path, path);
    ret->ssinfo = ssinfo;
    return ret;
}

void deleteCache(Cache c) {
    free(c);
}

void linkCache(Cache a, Cache b) {
    if (a) a->next = b;
    if (b) b->next = a;
}

void removeCache(Cache a) {
    // assert(lru.size > 0);
    linkCache(a->prev, a->next);
    if (lru.front == a) {
        lru.front = a->next;
    }

    if (lru.rear == a) {
        lru.rear = a->prev;
    }
    lru.size--;
}

void popBack() {
    Cache oldRear = lru.rear;
    removeCache(oldRear);
    deleteCache(oldRear);
}

void pushCacheFront(Cache a) {
    linkCache(a, lru.front);
    if (lru.rear == NULL) lru.rear = a;
    lru.front = a;
    lru.size++;
    if (lru.size > MAX_CACHE_SIZE) {
        popBack();
    }
}

void pushFront(char* path, SSInfo ssinfo) {
    Cache a = newCache(path, ssinfo);
    pushCacheFront(a);
}

void initLRU() {
    lru.size = 0;
    lru.front = lru.rear = NULL;
}

void destroyLRU() {
    for (Cache itr = lru.front; itr != NULL;) {
        Cache nxt = itr->next;
        deleteCache(itr);
        itr = nxt;
    }
}

bool isPrefix(char* prefix, char* string) {
    return strncmp(prefix, string, strlen(prefix)) == 0;
}

void deleteFromLRU(char* path) {
    for (Cache itr = lru.front; itr != NULL;) {
        Cache nxt = itr->next;
        if (isPrefix(path, itr->path)) {
            removeCache(itr);
            deleteCache(itr);
        }
        itr = nxt;
    }
}

SSInfo checkCache(char* path) {
    Cache hit = NULL;
    for (Cache itr = lru.front; itr != NULL;) {
        Cache nxt = itr->next;
        if (strcmp(path, itr->path) == 0) {
            hit = itr;
            break;
        }
        itr = nxt;
    }

    if (hit) {
        removeCache(hit);
        pushCacheFront(hit);
        return hit->ssinfo;
    } else {
        return INVALID_SSINFO;
    }
}

void removeSSFromLRU(SSInfo ssinfo) {
    for (Cache itr = lru.front; itr != NULL;) {
        Cache nxt = itr->next;
        if (SSInfoEqual(&ssinfo, &itr->ssinfo)) {
            removeCache(itr);
            deleteCache(itr);
            lru.size--;
        }
        itr = nxt;
    }
}