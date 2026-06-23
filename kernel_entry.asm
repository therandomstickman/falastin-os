[BITS 32]

global start
extern kmain

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x00000007   ; was 0x3, added bit 2 for video
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM
dd 0, 0, 0, 0, 0   ; load address fields (unused, set to 0)
dd 0               ; graphics mode: 0=linear framebuffer
dd 1024, 768, 32   ; width, height, depth

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text
start:
    mov esp, stack_top
    mov ebp, stack_top
    cli

    push ebx        ; push multiboot info pointer as argument
    call kmain

.hang:
    cli
    hlt
    jmp .hang