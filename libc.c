#include "libc.h"

// ── Memory ────────────────────────────────────────────────────────────────────

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void* memset(void* dest, int c, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < n; i++)
        d[i] = (uint8_t)c;
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--)
            d[i-1] = s[i-1];
    }
    return dest;
}

int memcmp(const void* a, const void* b, size_t n) {
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;
    for (size_t i = 0; i < n; i++) {
        if (pa[i] != pb[i])
            return pa[i] - pb[i];
    }
    return 0;
}

// ── String ────────────────────────────────────────────────────────────────────

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    char* d = dest + strlen(dest);
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        d[i] = src[i];
    d[i] = '\0';
    return dest;
}

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i])
            return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (c == '\0') ? (char*)s : 0;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return 0;
}

// ── Conversion ────────────────────────────────────────────────────────────────

int atoi(const char* s) {
    int result = 0;
    int sign   = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9')
        result = result * 10 + (*s++ - '0');
    return result * sign;
}

long atol(const char* s) {
    long result = 0;
    int  sign   = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9')
        result = result * 10 + (*s++ - '0');
    return result * sign;
}

char* itoa(int val, char* buf, int base) {
    char tmp[32];
    int  i   = 0;
    int  neg = 0;

    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return buf; }
    if (val < 0 && base == 10) { neg = 1; val = -val; }

    while (val > 0) {
        int rem = val % base;
        tmp[i++] = (rem < 10) ? '0' + rem : 'a' + rem - 10;
        val /= base;
    }
    if (neg) tmp[i++] = '-';

    // Reverse into buf
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
    return buf;
}

// ── Formatting ────────────────────────────────────────────────────────────────

// va_list implementation for freestanding environment
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

static int fmt_write_str(char* buf, size_t size, size_t* pos,
                          const char* s) {
    while (*s && *pos + 1 < size)
        buf[(*pos)++] = *s++;
    return 0;
}

static int fmt_write_int(char* buf, size_t size, size_t* pos,
                          long val, int base, int unsign, int pad, char padch) {
    char tmp[32];
    int  i   = 0;
    int  neg = 0;

    if (!unsign && val < 0) { neg = 1; val = -val; }
    unsigned long uval = (unsigned long)val;

    if (uval == 0) {
        tmp[i++] = '0';
    } else {
        while (uval > 0) {
            int rem = uval % base;
            tmp[i++] = (rem < 10) ? '0' + rem : 'a' + rem - 10;
            uval /= base;
        }
    }
    if (neg) tmp[i++] = '-';

    // Padding
    int len = i + (neg ? 0 : 0);
    while (pad > len && *pos + 1 < size) {
        buf[(*pos)++] = padch;
        pad--;
    }

    while (i > 0 && *pos + 1 < size)
        buf[(*pos)++] = tmp[--i];

    return 0;
}

int vsnprintf(char* buf, size_t size, const char* fmt, va_list ap) {
    size_t pos = 0;

    while (*fmt && pos + 1 < size) {
        if (*fmt != '%') {
            buf[pos++] = *fmt++;
            continue;
        }
        fmt++;

        char padch = ' ';
        int  pad   = 0;
        if (*fmt == '0') { padch = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9')
            pad = pad * 10 + (*fmt++ - '0');

        switch (*fmt++) {
            case 'd': case 'i':
                fmt_write_int(buf, size, &pos,
                    (long)va_arg(ap, int), 10, 0, pad, padch);
                break;
            case 'u':
                fmt_write_int(buf, size, &pos,
                    (long)(unsigned)va_arg(ap, unsigned int),
                    10, 1, pad, padch);
                break;
            case 'x':
                fmt_write_int(buf, size, &pos,
                    (long)(unsigned)va_arg(ap, unsigned int),
                    16, 1, pad, padch);
                break;
            case 'c': {
                char c = (char)va_arg(ap, int);
                if (pos + 1 < size) buf[pos++] = c;
                break;
            }
            case 's':
                fmt_write_str(buf, size, &pos, va_arg(ap, const char*));
                break;
            case '%':
                if (pos + 1 < size) buf[pos++] = '%';
                break;
            default:
                break;
        }
    }
    buf[pos] = '\0';
    return (int)pos;
}

int snprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}


int sprintf(char* buf, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = snprintf(buf, 4096, fmt, ap);  // BUG: passing va_list as arg
    va_end(ap);
    return ret;
}