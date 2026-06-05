; Simple test program for FalastinOS
; This writes to video memory directly

[BITS 32]

section .text
global _start

_start:
    ; Write a pattern to video memory
    mov edi, 0xB8000 + 160  ; Line 1, column 0
    mov ecx, 40              ; 40 characters
    
    mov al, 'A'
    mov ah, 0x0E             ; Yellow on black
    
write_loop:
    mov word [edi], ax
    add edi, 2
    inc al
    cmp al, 'Z' + 1
    jl next
    mov al, 'A'
next:
    loop write_loop
    
    ret