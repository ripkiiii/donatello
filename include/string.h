/* string.h — the tiny string toolkit. Freestanding kernels get no libc, so
 * even strcmp has to be ours. Only what the shell needs, nothing more. */
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen(const char* s);
int    strcmp(const char* a, const char* b);
int    strncmp(const char* a, const char* b, size_t n);

#endif
