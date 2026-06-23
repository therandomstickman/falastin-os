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

global irq0_handler
extern irq0_c

irq0_handler:
    pusha
    push ds
    push es

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call irq0_c

    mov al, 0x20
    out 0x20, al    ; EOI to master PIC

    pop es
    pop ds
    popa
    iretd

global irq12_handler
extern irq12_c

irq12_handler:
    pusha
    push ds
    push es

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call irq12_c

    ; Send EOI to both PICs (IRQ12 is on slave)
    mov al, 0x20
    out 0xA0, al   ; slave EOI
    out 0x20, al   ; master EOI

    pop es
    pop ds
    popa
    iretd