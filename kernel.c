#include "screen.h"
#include "shell.h"
#include "pic.h"
#include "interrupts.h"
#include "keyboard.h"
#include "cursor.h"
#include "fs.h"

void kmain(void)
{
    clear_screen();
    print("Initializing system...\n");
    
    // Enable the hardware cursor
    enable_cursor();
    
    // Initialize filesystem
    fs_init();
    print("Filesystem initialized\n");
    
    // Initialize IDT
    init_idt();
    print("IDT initialized\n");
    
    // Mask all IRQs
    pic_mask_all();
    
    // Enable only keyboard IRQ
    pic_unmask_irq(1);
    print("Keyboard enabled\n");
    
    // Enable interrupts
    asm volatile("sti");
    print("Interrupts enabled\n");
    
    print("\n=== FalastinOS Ready ===\n");
    print("Type 'help' for commands\n");
    print("Try: touch, ls, rm\n\n");
    
    shell_start();
}