.set FLAGS, 7
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)
.section .multiboot,"a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0
.long 1024
.long 768
.long 32
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
