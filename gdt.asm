[BITS 32]

global load_gdt
global gdt_flush

section .data

; GDT entry structure:
; Base 0-15, Base 16-23, Base 24-31, Limit 0-15, Limit 16-19, Access, Flags, 

gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0

; Code segment descriptor (0x08)
gdt_code:
    dw 0xFFFF    ; Limit 0-15
    dw 0x0000    ; Base 0-15
    db 0x00      ; Base 16-23
    db 10011010b ; Access: Present, Ring0, Code, Non-conforming, Readable
    db 11001111b ; Flags + Limit 16-19: 4KB pages, 32-bit, Limit=0xFFFFF
    db 0x00      ; Base 24-31

; Data segment descriptor (0x10)
gdt_data:
    dw 0xFFFF    ; Limit 0-15
    dw 0x0000    ; Base 0-15
    db 0x00      ; Base 16-23
    db 10010010b ; Access: Present, Ring0, Data, Expand-down, Writable
    db 11001111b ; Flags + Limit 16-19: 4KB pages, 32-bit, Limit=0xFFFFF
    db 0x00      ; Base 24-31

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

section .text

load_gdt:
    lgdt [gdt_descriptor]
    ret

gdt_flush:
    ; Reload segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload CS
    jmp 0x08:.flush
.flush:
    ret