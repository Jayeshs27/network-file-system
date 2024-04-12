#include "trie.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_SEPARATOR "/"

typedef struct TrieNodeStruct {
    struct TrieNodeStruct* parent;
    struct TrieNodeStruct* next;
    struct TrieNodeStruct* prev;
    struct TrieNodeStruct* firstChild;
    char* token;
    SSInfo ssinfo;
    pthread_rwlock_t rwLock;
} TrieNodeStruct;

typedef TrieNodeStruct* TrieNode;

// UTIL FUNCTIONS

TrieNode trie = NULL;
pthread_mutex_t lock;

TrieNode newNode(SSInfo ssinfo, char* token) {
    TrieNode ret = malloc(sizeof(TrieNodeStruct));
    assert(ret);
    ret->parent = NULL;
    ret->next = NULL;
    ret->prev = NULL;
    ret->firstChild = NULL;
    ret->token = strdup(token);
    ret->ssinfo = ssinfo;
    pthread_rwlock_init(&ret->rwLock, NULL);
    return ret;
};

void deleteNode(TrieNode node) {
    pthread_rwlock_destroy(&node->rwLock);
    free(node->token);
    free(node);
}

void link(TrieNode a, TrieNode b) {
    if (a) a->next = b;
    if (b) b->prev = a;
}

void appendChild(TrieNode parent, TrieNode child) {
    assert(child);
    assert(parent);
    link(child, parent->firstChild);
    parent->firstChild = child;
    child->parent = parent;
}

// Removes itr from its links
void removeChild(TrieNode itr) {
    link(itr->prev, itr->next);
    deleteNode(itr);
}

// Returns NULL if none found
TrieNode findChild(TrieNode parent, char* token) {
    for (TrieNode itr = parent->firstChild; itr != NULL; itr = itr->next) {
        if (strcmp(itr->token, token)) continue;
        return itr;
    }

    return NULL;
}

TrieNode searchNode(char* path) {
    TrieNode parent = trie;
    char* cpy = strdup(path);
    char* token = strtok(cpy, FILE_SEPARATOR);
    while (token != NULL) {
        TrieNode child = findChild(parent, token);
        if (child == NULL) {
            parent = NULL;
            goto clean_and_return;
        }
        parent = child;
        token = strtok(NULL, FILE_SEPARATOR);
    }
clean_and_return:
    free(cpy);
    return parent;
}

// EXPOSED FUNCTIONS

void initTrie() {
    SSInfo ssinfo = INVALID_SSINFO;
    trie = newNode(ssinfo, "");
    pthread_mutex_init(&lock, NULL);
}

void lockTrie() {
    pthread_mutex_lock(&lock);
}

void unlockTrie() {
    pthread_mutex_unlock(&lock);
}

ErrorCode addToTrie(char* path, SSInfo ssinfo) {
    TrieNode find = searchNode(path);
    if (find != NULL) return 3;
    TrieNode parent = trie;
    char* cpy = strdup(path);
    char* token = strtok(cpy, FILE_SEPARATOR);
    while (token != NULL) {
        TrieNode child = findChild(parent, token);
        if (child == NULL) {
            child = newNode(ssinfo, token);
            appendChild(parent, child);
        }
        parent = child;
        token = strtok(NULL, FILE_SEPARATOR);
    }
    free(cpy);
    return SUCCESS;
}

ErrorCode search(char* path, SSInfo* ret) {
    SSInfo cache = checkCache(path);
    if (cache.ssClientPort == -1) {
        *ret = INVALID_SSINFO;
        TrieNode node = searchNode(path);
        if (node) {
            *ret = node->ssinfo;
            pushFront(path, *ret);
        } else {
            return EPATHNOTFOUND;
        }
    } else {
        *ret = cache;
    }

    return SUCCESS;
}

void deleteNodeFromTrie(TrieNode node) {
    TrieNode parent = node->parent;
    if (parent && parent->firstChild == node) {
        parent->firstChild = node->next;
    }

    removeChild(node);
}

void destroyTrieHelper(TrieNode node) {
    if (node == NULL) return;
    for (TrieNode itr = node->firstChild; itr != NULL;) {
        TrieNode next = itr->next;
        destroyTrieHelper(itr);
        itr = next;
    }

    deleteNodeFromTrie(node);
}

ErrorCode deleteFromTrie(char* path) {
    TrieNode node = searchNode(path);
    if (node == NULL) {
        return 1;
    }

    deleteFromLRU(path);

    destroyTrieHelper(node);
    return SUCCESS;
}

void destroySSFromTrieHelper(TrieNode node, SSInfo ssinfo) {
    if (node == NULL) return;
    for (TrieNode itr = node->firstChild; itr != NULL;) {
        TrieNode next = itr->next;
        destroySSFromTrieHelper(itr, ssinfo);
        itr = next;
    }

    if (SSInfoEqual(&node->ssinfo, &ssinfo)) deleteNodeFromTrie(node);
}

char** getChildren(char* path, int* count) {
    *count = 0;
    TrieNode node = searchNode(path);
    if (node == NULL) {
        *count = -1;
        return NULL;
    }

    for (TrieNode itr = node->firstChild; itr != NULL;) {
        TrieNode nxt = itr->next;
        *count += 1;
        itr = nxt;
    }

    char** ret = malloc(sizeof(char*) * *count);

    int i = 0;
    for (TrieNode itr = node->firstChild; itr != NULL; i++) {
        TrieNode nxt = itr->next;
        ret[i] = strdup(itr->token);
        itr = nxt;
    }
    return ret;
}

ErrorCode deleteSSFromTrie(SSInfo ssinfo) {
    removeSSFromLRU(ssinfo);
    destroySSFromTrieHelper(trie, ssinfo);
    return SUCCESS;
}

void destroyTrie() {
    pthread_mutex_destroy(&lock);
    destroyLRU();
    destroyTrieHelper(trie);
}

void readLockPath(char* path) {
    TrieNode node = searchNode(path);
    assert(node);
    pthread_rwlock_rdlock(&node->rwLock);
}

void readUnlockPath(char* path) {
    TrieNode node = searchNode(path);
    assert(node);
    pthread_rwlock_unlock(&node->rwLock);
}

void writeLockPath(char* path) {
    TrieNode node = searchNode(path);
    assert(node);
    pthread_rwlock_wrlock(&node->rwLock);
}

void writeUnlockPath(char* path) {
    TrieNode node = searchNode(path);
    assert(node);
    pthread_rwlock_unlock(&node->rwLock);
}