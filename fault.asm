[BITS 32]

global fault_handler
extern print

fault_handler:
    cli
    pusha
    
    push msg
    call print
    add esp, 4
    
    popa
    hlt

section .rodata
msg db "Exception occurred!", 0