# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

.globl swtch
swtch:
  movq 8(%esp), %rax
  movq 16(%esp), %rdx

  # Save old callee-saved registers
  pushq %rbp
  pushq %rbx
  pushq %rsi
  pushq %rdi

  # Switch stacks
  movq %rsp, (%eax)
  movq %rdx, %esp

  # Load new callee-saved registers
  popq %rdi
  popq %rsi
  popq %rbx
  popq %rbp
  ret
