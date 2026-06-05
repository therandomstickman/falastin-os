#include "cursor.h"

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void update_cursor(int row, int col)
{
    uint16_t position = row * 80 + col;
    
    outb(0x3D4, 0x0E);
    outb(0x3D5, (position >> 8) & 0xFF);
    outb(0x3D4, 0x0F);
    outb(0x3D5, position & 0xFF);
}

void enable_cursor(void)
{
    outb(0x3D4, 0x0A);
    uint8_t cursor_start = 0x0E;
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    
    outb(0x3D4, 0x0B);
    uint8_t cursor_end = 0x0F;
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}