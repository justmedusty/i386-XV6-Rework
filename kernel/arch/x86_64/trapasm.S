#include "mem/mmu.h"

# vectors.S sends all traps here.
.globl alltraps
alltraps:
# Build trap frame.
pushq %rax
        pushq %rbx
        pushq %rcx
        pushq %rdx
        pushq %rsi
        pushq %rdi
        pushq %rbp
        pushq %r8
        pushq %r9
        pushq %r10
        pushq %r11
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15

# Set up data segments.
# In 64-bit mode, segments are typically unused or set to flat
# but for completeness if needed, use movq to move 64-bit values

# We typically don't need to set %ds and %es in 64-bit mode
# Example for completeness, but not usually required:
# movq $0x10, %rax # KDATA segment selector if needed
# mov %ax, %ds
# mov %ax, %es

# Call trap(tf), where tf=%rsp
        mov %rsp, %rdi  # First argument in rdi
call trap

# Return falls through to trapret...
.globl trapret
trapret:
popq %r15
        popq %r14
        popq %r13
        popq %r12
        popq %r11
        popq %r10
        popq %r9
        popq %r8
        popq %rbp
        popq %rdi
        popq %rsi
        popq %rdx
        popq %rcx
        popq %rbx
        popq %rax
        addq $0x10, %rsp  # trapno and errcode, each 8 bytes in 64-bit mode
iretq  # Use iretq in 64-bit mode