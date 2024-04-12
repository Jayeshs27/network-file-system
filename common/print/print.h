#ifndef __PRINT_H
#define __PRINT_H

#include <stdio.h>

#include "colors.h"

void cvfprintf(FILE* stream, char* const color, const char* format, va_list arguments);
void cfprintf(FILE* stream, char* const color, const char* format, ...);
void cprintf(char* const color, const char* format, ...);

#endif