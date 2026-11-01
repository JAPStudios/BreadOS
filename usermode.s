.code32
.section .text
.global user_enter
.type user_enter, @function
.extern gdt_set_kernel_stack
user_enter:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    pushl %edi
    movl %esp, %eax
    pushl %eax
    call gdt_set_kernel_stack
    addl $4, %esp

    movl 8(%ebp), %eax
    movl 12(%ebp), %edx
    movw $0x23, %cx
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    pushl $0x23
    pushl %edx
    pushfl
    popl %ecx
    orl $0x200, %ecx
    pushl %ecx
    pushl $0x1B
    pushl %eax
    iret

.global user_exit_trampoline
.type user_exit_trampoline, @function
user_exit_trampoline:
    addl $8, %esp
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret
.section .note.GNU-stack,"",@progbits
