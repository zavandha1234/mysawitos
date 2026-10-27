; My Sawit OS v1.0 - boot.asm (Multiboot 1, minta framebuffer linear 640x480x32)
bits 32
MB_MAGIC equ 0x1BADB002
MB_FLAGS equ 0x00000007          ; bit0 align modul, bit1 mem info, bit2 video mode
section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd -(MB_MAGIC + MB_FLAGS)
    dd 0, 0, 0, 0, 0             ; header_addr, load_addr, load_end, bss_end, entry (dipakai ELF)
    dd 0                         ; mode_type: 0 = linear graphics
    dd 640                       ; width
    dd 480                       ; height
    dd 32                        ; depth

section .text
global _start
extern kmain
_start:
    cli
    mov esp, stack_top
    push ebx                     ; arg2: multiboot info
    push eax                     ; arg1: magic
    call kmain
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom: resb 65536
stack_top:
