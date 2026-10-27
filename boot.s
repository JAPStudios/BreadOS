.set FLAGS, 3
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
.section .text
.global _start
.type _start, @function
.extern kernel_main
_start:
 cli
 mov $stack_top, %esp
 xor %ebp, %ebp
 push %ebx
 push %eax
 call kernel_main
1: cli
 hlt
 jmp 1b
