#include "ssinfo.h"

void initSSInfo(SSInfo* ssinfo, int ssClientPort, int ssPassivePort) {
    ssinfo->ssClientPort = ssClientPort;
    ssinfo->ssPassivePort = ssPassivePort;
}

bool SSInfoEqual(SSInfo* a, SSInfo* b) {
    return a->ssClientPort == b->ssClientPort && a->ssPassivePort == b->ssPassivePort;
}