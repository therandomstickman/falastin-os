#include "timer.h"
#include "pic.h"

static volatile uint32_t ticks = 0;
static uint32_t tick_rate = 100;  // hz

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void irq0_c(void) {
    ticks++;
}

void timer_init(uint32_t hz) {
    tick_rate = hz;
    uint32_t divisor = 1193180 / hz;

    outb(0x43, 0x36);                        // channel 0, lobyte/hibyte, mode 3
    outb(0x40, (uint8_t)(divisor & 0xFF));   // low byte
    outb(0x40, (uint8_t)(divisor >> 8));     // high byte
}

uint32_t timer_ticks(void) {
    return ticks;
}

void timer_sleep(uint32_t ms) {
    uint32_t target = ticks + (ms * tick_rate / 1000);
    while (ticks < target)
        asm volatile("hlt");
}