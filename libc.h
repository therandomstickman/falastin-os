#ifndef LIBC_H
#define LIBC_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// Memory
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* dest, int c, size_t n);
void* memmove(void* dest, const void* src, size_t n);
int   memcmp(const void* a, const void* b, size_t n);

// String
size_t strlen(const char* s);
char*  strcpy(char* dest, const char* src);
char*  strncpy(char* dest, const char* src, size_t n);
char*  strcat(char* dest, const char* src);
char*  strncat(char* dest, const char* src, size_t n);
int    strcmp(const char* a, const char* b);
int    strncmp(const char* a, const char* b, size_t n);
char*  strchr(const char* s, int c);
char*  strstr(const char* haystack, const char* needle);

// Formatting
int sprintf(char* buf, const char* fmt, ...);
int snprintf(char* buf, size_t size, const char* fmt, ...);
int vsnprintf(char* buf, size_t size, const char* fmt, va_list ap);

// Conversion
int    atoi(const char* s);
long   atol(const char* s);
char*  itoa(int val, char* buf, int base);

#endif