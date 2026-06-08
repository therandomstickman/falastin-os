#include "pic.h"

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

void pic_remap(void)
{
    // Save masks
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    
    // Start initialization in cascade mode
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    
    // Set vector offsets
    outb(0x21, 0x20);  // Master: IRQ0-7 -> INT 0x20-0x27
    outb(0xA1, 0x28);  // Slave:  IRQ8-15 -> INT 0x28-0x2F
    
    // Tell master about slave (IRQ2)
    outb(0x21, 0x04);
    // Tell slave its cascade identity
    outb(0xA1, 0x02);
    
    // Set 8086 mode
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    // Restore saved masks
    outb(0x21, a1);
    outb(0xA1, a2);
}

void pic_mask_all(void)
{
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
        value = inb(port);
        value &= ~(1 << irq);
        outb(port, value);
    } else {
        port = 0xA1;
        value = inb(port);
        value &= ~(1 << (irq - 8));
        outb(port, value);
    }
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}