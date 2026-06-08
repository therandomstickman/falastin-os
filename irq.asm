[BITS 32]

global irq1_handler
extern irq1_c

section .text

irq1_handler:
    ; Save registers
    pusha
    push ds
    push es
    
    ; Set kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    
    ; Call C handler
    call irq1_c
    
    ; Send EOI to master PIC (for IRQ1)
    mov al, 0x20
    out 0x20, al
    
    ; Restore registers
    pop es
    pop ds
    popa
    
    ; Return from interrupt
    iretd