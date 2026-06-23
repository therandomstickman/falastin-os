#include "screen.h"
#include "shell.h"
#include "pic.h"
#include "interrupts.h"
#include "keyboard.h"
#include "cursor.h"
#include "fs.h"
#include "graphics.h"
#include "font.h"
#include "mouse.h"
#include "cursor_gfx.h"
#include "brickwm.h"
#include "progman.h"
#include "ata.h"
#include "malloc.h"
#include "timer.h"
#include "net.h"


// Forward declaration
void terminal_run(void);

// ... rest of kernel.c
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} __attribute__((packed)) MultibootInfo;

void kmain(MultibootInfo* mb_info) {
    heap_init();
    if (mb_info->flags & (1 << 12)) {
        graphics_init(
            (uint32_t)mb_info->framebuffer_addr,
            mb_info->framebuffer_pitch,
            mb_info->framebuffer_width,
            mb_info->framebuffer_height,
            mb_info->framebuffer_bpp
        );
        font_init();
        fill_screen(0x00001133);
    }

    init_idt();

    mouse_init();
    timer_init(100);      // 100hz = 10ms per tick
    pic_unmask_irq(0);    // unmask timer IRQ

    pic_unmask_irq(1);   // keyboard
    pic_unmask_irq(2);   // cascade
    pic_unmask_irq(12);  // mouse

    asm volatile("sti");
    print("=== KERNEL BOOT ===\n");
    print("Testing print: If you see this, output works!\n");
    cursor_gfx_draw(512, 384);

    enable_cursor();
    ata_init();
    net_init();
    fs_init();

    clear_screen();

    print("FalastinOS V0.2\n");
    print("(c) 2026 therandomstickman. All rights reserved.\n");
    print("\n");

    //shell_start();
    progman_run();
}