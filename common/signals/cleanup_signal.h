#ifndef __CLEANUP_SIGNAL_H
#define __CLEANUP_SIGNAL_H

#include <signal.h>

void initEscapeHatch(void (*cleanup)());
void destroyEscapeHatch();

#endif