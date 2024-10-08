/**
 * @file  kernel/arch/x86_64/bootstrap.c
 * @brief x86-64 entrypoint from bootloader
 *
 * This is primarily adapted from Sirius's 32-bit mulitboot bootstrap.
 * Instead of jumping straight to our C entry point, however, we need
 * to (obviously) get ourselves set up for long mode first by setting
 * up initial page tables, etc.
 *
 * @copyright
 * This file is part of SiriusOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2021 K. Lange
 */
.section .multiboot
.code32

.extern bss_start
.extern end
.extern phys

/* Multiboot 1 header */
.set MB_MAGIC,              0x1BADB002
.set MB_FLAG_PAGE_ALIGN,    1 << 0
.set MB_FLAG_MEMORY_INFO,   1 << 1
.set MB_FLAG_GRAPHICS,      1 << 2
.set MB_FLAG_AOUT,          1 << 16
.set MB_FLAGS,              MB_FLAG_PAGE_ALIGN | MB_FLAG_MEMORY_INFO | MB_FLAG_GRAPHICS | MB_FLAG_AOUT
.set MB_CHECKSUM,           -(MB_MAGIC + MB_FLAGS)

/* Multiboot section */
.align 4
multiboot_header:
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECKSUM
.long multiboot_header /* header_addr */
.long phys             /* load_addr */
.long bss_start        /* load_end_addr */
.long end              /* bss_end_addr */
.long start            /* entry_addr */

/* Request linear graphics mode */
.long 0x00000000
.long 1024
.long 768
.long 32

/* Multiboot 2 header */
.set MB2_MAGIC,    0xe85250d6
.set MB2_ARCH,     0
.set MB2_LENGTH,   (multiboot2_header_end - multiboot2_header)
.set MB2_CHECKSUM, -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)
.align 8
multiboot2_header:
.long MB2_MAGIC
.long MB2_ARCH
.long MB2_LENGTH
.long MB2_CHECKSUM

/* Address tag */
.align 8
mb2_tag_addr:
.word 2
.word 0
.long 24
.long multiboot2_header
.long phys
.long bss_start
.long end

/* Entry tag */
.align 8
mb2_tag_entry:
.word 3
.word 0
.long 12
.long start_mbi2

/* Framebuffer tag */
.align 8
mb2_tag_fb:
.word 5
.word 0
.long 20
.long 1024
.long 768
.long 32

.align 8
.word 4
.word 1
.long 12
.long 2 /* We support EGA text, but don't require it */

/* Modules must be aligned */
.align 8
.word 6
.word 0
.long 8

/* Relocatable tag */
.align 8
.word 10
.word 0
.long 24
.long 0x100000  /* Start */
.long 0x1000000 /* Maximum load address */
.long 4096      /* Request page alignment */
.long 1         /* Load at lowest available */

/* End tag */
.align 8
.word 0
.word 0
.long 8

multiboot2_header_end:

/* .stack resides in .bss */
.section .stack, "aw", @nobits
stack_bottom:
.skip 16384 /* 16KiB */
.global stack_top
stack_top:

.section .bootstrap
.code32
.align 4

.extern jmp_to_long
.type jmp_to_long, @function

.extern kmain
.type kmain, @function

.global start_mbi2
.type start_mbi2, @function

start_mbi2:
    /* Use reserved 0 space of boot information struct to thunk eip */
    movl %ebx, %ecx
    addl $8, %ecx
    jmp good_to_go

.global start
.type start, @function

start:
    /* Use boot_drive as a temporary place to thunk eip */
    movl %ebx, %ecx
    addl $16, %ecx

good_to_go:
    mov  %ecx, %esp

    call _forward
_forward:
    popl %ecx
    subl $_forward, %ecx

    /* Setup our stack */
    mov $stack_top, %esp
    addl %ecx, %esp

    /* Make sure our stack is 16-byte aligned */
    and $-16, %esp

    pushl $0
    pushl %esp
    pushl $0
    pushl %eax /* Multiboot header magic */
    pushl $0
    pushl %ebx /* Multiboot header pointer */

    jmp jmp_to_long


.align 4

jmp_to_long:
    .extern init_page_region

    /* Set up initial page region, which was zero'd for us by the loader */
    mov $init_page_region, %edi
    addl %ecx, %edi
    pushl %ecx

    /* PML4[0] = &PDP[0] | (PRESENT, WRITABLE, USER) */
    mov $0x1007, %eax
    add %edi, %eax
    mov %eax, (%edi)

    /* PDP[0] = &PD[0] | (PRESENT, WRITABLE, USER) */
    add $0x1000, %edi
    mov $0x1003, %eax
    add %edi, %eax
    mov %eax, (%edi)

    /* Set 32 2MiB pages to map 64MiB of low memory temporarily, which should
       be enough to get us through our C MMU initialization where we then
       use 2MiB pages to map all of the 4GiB standard memory space and map
       a much more restricted subset of the kernel in the lower address space. */
    add $0x1000, %edi

    mov $0x87, %ebx
    mov $32, %ecx

.set_entry:
    mov %ebx, (%edi)
    add $0x200000, %ebx
    add $8, %edi
    loop .set_entry

    mov $init_page_region, %edi
    popl %ecx
    addl %ecx, %edi
    pushl %ecx
    mov %edi, %cr3

    /* Enable PAE */
    mov %cr4, %eax
    or $32, %eax
    mov %eax, %cr4

    /* EFER */
    mov $0xC0000080, %ecx
    rdmsr
    or $256, %eax
    wrmsr

    /* Check PG */
    mov %cr0, %eax

    /* If paging was enabled, assume we were already in long mode (eg. booted by EFI) */
    test $0x80000000, %eax
    jnz .continue

    /* Otherwise enable paging */
    or $0x80000000, %eax
    mov %eax, %cr0

super_duper_bullshit:
    popl %ecx

    lea (_lgdt_instr+3)(%ecx), %eax
    movl (%eax), %ebx
    addl %ecx, %ebx
    movl %ebx, (%eax)

    lea (gdtr+2)(%ecx), %eax
    movl (%eax), %ebx
    addl %ecx, %ebx
    movl %ebx, (%eax)

    lea (_ljmp_instr+1)(%ecx), %eax
    movl (%eax), %ebx
    addl %ecx, %ebx
    movl %ebx, (%eax)

    pushl $0
    pushl %ecx

_lgdt_instr:
    lgdt gdtr

_ljmp_instr:
    ljmp $0x08,$realm64

.align 8
gdtr:
    .word gdt_end-gdt_base
    .quad gdt_base

gdt_base:
    /* Null */
    .quad 0
    /* Code */
    .word 0
    .word 0
    .byte 0
    .byte 0x9a
    .byte 0x20
    .byte 0
    /*  Data */
    .word 0xffff
    .word 0
    .byte 0
    .byte 0x92
    .byte 0
    .byte 0
gdt_end:


.code64
.align 8
.section .bootstrap

realm64:
    cli
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

.continue:
    cli
    pop %rcx
    pop %rdi
    pop %rsi
    pop %rdx
    callq kmain

halt:
    cli
    hlt
    jmp halt
