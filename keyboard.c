#include <stdint.h>
#include "keyboard.h"
#include "screen.h"

static volatile char last_char = 0;
static volatile int char_ready = 0;
static volatile int shift_pressed = 0;
static volatile int ctrl_pressed = 0;

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

void irq1_c(void)
{
    uint8_t scancode = inb(0x60);
    
    // Handle modifier keys
    if (scancode == 0x2A || scancode == 0x36) {  // Left or Right Shift press
        shift_pressed = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {  // Left or Right Shift release
        shift_pressed = 0;
        return;
    }
    if (scancode == 0x1D) {  // Left Ctrl press
        ctrl_pressed = 1;
        return;
    }
    if (scancode == 0x9D) {  // Left Ctrl release
        ctrl_pressed = 0;
        return;
    }
    
    // Only handle key press (not release)
    if (!(scancode & 0x80)) {
        if (scancode < sizeof(scancode_to_ascii)) {
            char c = 0;
            
            if (ctrl_pressed && !shift_pressed) {
                // Ctrl+key produces control characters (1-26)
                char base = scancode_to_ascii[scancode];
                if (base >= 'a' && base <= 'z') {
                    c = base - 'a' + 1;
                } else if (base >= 'A' && base <= 'Z') {
                    c = base - 'A' + 1;
                }
            }
            
            if (c == 0) {
                // Regular key handling with shift
                if (shift_pressed) {
                    c = scancode_to_ascii_shift[scancode];
                } else {
                    c = scancode_to_ascii[scancode];
                }
            }
            
            if (c) {
                last_char = c;
                char_ready = 1;
            }
        }
    }
}

char keyboard_getchar(void)
{
    while (!char_ready);
    char_ready = 0;
    return last_char;
}