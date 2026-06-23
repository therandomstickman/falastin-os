#include "mouse.h"
#include "graphics.h"
#include "cursor_gfx.h"
#include "pic.h"
#include "screen.h"
#include "fs.h"

#define MOUSE_PORT      0x60
#define MOUSE_STATUS    0x64

static int mx = 512;
static int my = 384;
static int mbuttons = 0;

static uint8_t mouse_byte[3];
static int mouse_byte_idx = 0;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void mouse_wait(void) {
    for (int i = 0; i < 100000; i++)
        if (!(inb(MOUSE_STATUS) & 0x02)) return;
}

static void mouse_wait_data(void) {
    for (int i = 0; i < 100000; i++)
        if (inb(MOUSE_STATUS) & 0x01) return;
}

static uint8_t mouse_read(void) {
    mouse_wait_data();
    return inb(MOUSE_PORT);
}

static void mouse_write(uint8_t data) {
    mouse_wait();
    outb(MOUSE_STATUS, 0xD4);
    mouse_wait();
    outb(MOUSE_PORT, data);
}

void mouse_init(void) {
    print("Initializing PS/2 mouse...\n");
    
    // Enable auxiliary device
    mouse_wait();
    outb(MOUSE_STATUS, 0xA8);
    
    // Enable interrupts
    mouse_wait();
    outb(MOUSE_STATUS, 0x20);
    mouse_wait_data();
    uint8_t status = inb(MOUSE_PORT);
    status |= 0x02;
    mouse_wait();
    outb(MOUSE_STATUS, 0x60);
    mouse_wait();
    outb(MOUSE_PORT, status);
    
    // Set to standard PS/2 mode
    mouse_write(0xF6);  // Set defaults
    mouse_read();       // ACK
    
    // Enable streaming mode
    mouse_write(0xEA);  // Set stream mode
    mouse_read();       // ACK
    
    // Enable data reporting
    mouse_write(0xF4);  // Enable
    mouse_read();       // ACK
    
    mouse_byte_idx = 0;
    print("Mouse ready\n");
}

void irq12_c(void) {
    uint8_t data = inb(MOUSE_PORT);
    
    // Synchronize: First byte of packet must have bit 3 = 1
    if (mouse_byte_idx == 0 && !(data & 0x08)) {
        // Not a valid first byte, discard and wait for sync
        return;
    }
    
    mouse_byte[mouse_byte_idx] = data;
    mouse_byte_idx++;
    
    if (mouse_byte_idx == 3) {
        mouse_byte_idx = 0;
        
        // Extract buttons
        mbuttons = mouse_byte[0] & 0x07;
        
        // Get movement (as signed bytes)
        int dx = (int8_t)mouse_byte[1];
        int dy = (int8_t)mouse_byte[2];
        
        // Update position (Y is inverted on PS/2)
        mx -= dx;
        my += dy;
        
        // Clamp to screen bounds
        int w = (int)fb_width_get();
        int h = (int)fb_height_get();
        if (mx < 0) mx = 0;
        if (mx >= w) mx = w - 1;
        if (my < 0) my = 0;
        if (my >= h) my = h - 1;
        
        // The window manager owns cursor drawing. Drawing from the IRQ can
        // race with full-screen repaints and leave stale cursor pixels behind.
    }
}

int mouse_x(void)       { return mx; }
int mouse_y(void)       { return my; }
int mouse_buttons(void) { return mbuttons; }
