#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdint.h>
#include <stddef.h>

// No system ctype.h on bare metal — use lwIP's built-in character macros
#define LWIP_NO_CTYPE_H 1
typedef uint8_t   u8_t;
typedef int8_t    s8_t;
typedef uint16_t  u16_t;
typedef int16_t   s16_t;
typedef uint32_t  u32_t;
typedef int32_t   s32_t;

typedef uintptr_t mem_ptr_t;

// Declare print function from screen.h
extern void print(const char* str);

// Compiler hints
#define LWIP_PROVIDE_ERRNO 1

// Packing
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

// Align
#define LWIP_PLATFORM_ALIGNMENT 4

// Endianness (little endian)
#define BYTE_ORDER LITTLE_ENDIAN

// String functions - disable debug output to avoid formatting issues
#define LWIP_PLATFORM_DIAG(x) do { } while(0)

// Assertion - simple version
#define LWIP_PLATFORM_ASSERT(x) do { print("Assertion failed: "); print(x); print("\n"); while(1); } while(0)

// Random number for DNS
#define LWIP_RAND() ((u32_t)42)  // Simple fixed value for now

// Memory functions
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif /* LWIP_ARCH_CC_H */