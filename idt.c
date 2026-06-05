#include "idt.h"
#include "pic.h"

#define IDT_SIZE 256

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[IDT_SIZE];
struct idt_ptr idtp;

extern void irq0_handler(void);
extern void irq1_handler(void);
extern void load_idt(uint32_t);

static void set_gate(int i, uint32_t handler)
{
    idt[i].base_low  = handler & 0xFFFF;
    idt[i].base_high = (handler >> 16) & 0xFFFF;

    idt[i].sel = 0x08;
    idt[i].always0 = 0;
    idt[i].flags = 0x8E;
}

void init_idt(void)
{
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < IDT_SIZE; i++)
        set_gate(i, 0);

    set_gate(0x20, (uint32_t)irq0_handler);
    set_gate(0x21, (uint32_t)irq1_handler);

    pic_remap();
    pic_mask_all();   // IMPORTANT: start safe

    load_idt((uint32_t)&idtp);
}