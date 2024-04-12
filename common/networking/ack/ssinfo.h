#ifndef __SSINFO_H
#define __SSINFO_H

#include <stdbool.h>

typedef struct SSInfo {
    int ssClientPort;
    int ssPassivePort;
} SSInfo;

void initSSInfo(SSInfo* ssinfo, int ssClientPort, int ssPassivePort);
bool SSInfoEqual(SSInfo* a, SSInfo* b);
#define INVALID_SSINFO (SSInfo){-1, -1};

#endif