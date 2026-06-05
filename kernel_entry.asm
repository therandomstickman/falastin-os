[BITS 32]

global start
extern kmain

MULTIBOOT_MAGIC equ 0x1BADB002
MULTIBOOT_FLAGS equ 0x00000003
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text
start:
    mov esp, stack_top
    mov ebp, stack_top  ; Set up base pointer too
    
    ; Clear interrupts while setting up
    cli
    
    call kmain

.hang:
    cli
    hlt
    jmp .hang