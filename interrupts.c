#include <stdint.h>
#include "interrupts.h"
#include "pic.h"

#define IDT_SIZE 256

struct idt_entry
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_SIZE];
static struct idt_ptr idtp;

extern void load_idt(uint32_t);
extern void load_gdt(void);
extern void gdt_flush(void);
extern void irq1_handler(void);
extern void irq12_handler(void);
extern void irq0_handler(void);

static void set_gate(int num, uint32_t handler, uint16_t selector, uint8_t flags)
{
    idt[num].base_low = handler & 0xFFFF;
    idt[num].base_high = (handler >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void init_idt(void)
{
    // Load GDT first!
    load_gdt();
    gdt_flush();
    
    // Clear IDT
    for(int i = 0; i < IDT_SIZE; i++) {
        set_gate(i, 0, 0, 0);
    }
    
    // Remap PIC
    pic_remap();
    
    // Mask all IRQs initially
    pic_mask_all();
    
    // Set keyboard interrupt gate
    set_gate(0x21, (uint32_t)irq1_handler, 0x08, 0x8E);
    set_gate(0x2C, (uint32_t)irq12_handler, 0x08, 0x8E);
    set_gate(0x20, (uint32_t)irq0_handler, 0x08, 0x8E);
    
    
    // Load IDT
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)idt;
    load_idt((uint32_t)&idtp);
}