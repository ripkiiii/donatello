/* string.c — hand-rolled string functions. Same contracts as the C library:
 * strcmp returns 0 on equal, negative/positive by first differing byte. */

#include "string.h"

size_t strlen(const char* s) {
	size_t n = 0;
	while (s[n])
		n++;
	return n;
}

int strcmp(const char* a, const char* b) {
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
	while (n && *a && *a == *b) {
		a++;
		b++;
		n--;
	}
	if (n == 0)
		return 0;
	return (unsigned char)*a - (unsigned char)*b;
}
