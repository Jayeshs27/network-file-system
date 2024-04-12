#include "print.h"
#include "colors.h"

#include <stdarg.h>

void cvfprintf(FILE* stream, char* const color, const char* format, va_list arguments) {
    fprintf(stream, color);
    vfprintf(stream, format, arguments);
    fprintf(stream, COLOR_RESET);
}

void cfprintf(FILE* stream, char* const color, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    cvfprintf(stream, color, format, arguments);
}

void cprintf(char* const color, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    cvfprintf(stdout, color, format, arguments);
}