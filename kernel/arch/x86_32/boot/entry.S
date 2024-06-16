# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.
        
# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "../asm.h"
#include "../mem/memlayout.h"
#include "../mem/mmu.h"
#include "../../../defs/param.h"

# Multiboot header.  Data to direct multiboot loader.
.p2align 2
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002
  #define flags 0
  .long magic
  .long flags
  .long (-magic-flags)

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
_start = V2P_WO(entry)

# Entering xv6 on boot processor, with paging off.
.globl entry
entry:

  # Turn on page size extension for 4Mbyte pages
  mov     eax, cr4
  or     eax, $(CR4_PSE)
  mov    cr4, eax

  # Set page directory
  mov    eax, $(V2P_WO(entrypgdir))
  mov    cr3, eax

# Turn on paging.
#  mov    %cr0, %eax
#  orl     $(CR0_PG|CR0_WP), %eax
#  mov    %eax, %cr0

  #Detect long mode
  mov 0x80000000, eax
  cpuid
  cmp %eax, 0x80000001
  jb .done

  #Extended is available, check for long mode
  mov eax, 0x80000001
  cpuid
  test 1 << 29, edx
  jb .done

  #Long mode is supported, we can use it
  mov eax, cr0
  and eax, 0b1111111111111111111111111111111,
  mov cr0, eax

  #identity map the first mb
  mov edi, 0x1000
  mov cr3, edi
  xor eax, eax
  mov ecx, 4096
  rep stosd
  mov edi, cr3

  #fill up the page table
  mov $0x2003, %edi
  addl $0x1000, %edi
  mov $0x3003, %edi
  addl $0x1000, %edi
  mov $0x4003, %edi
  addl $0x1000, %edi

  mov 0x00000003, %ebx
  mov 512, %ecx

.SetEntry:
  movl %ebx, %edi
  addl 0x1000, %ebx
  addl 8, %edi
  loop .SetEntry

  #enable PAE
  mov %cr4, %eax
  orl 1 << 5, %eax
  mov %eax, %cr4

  #get out of protected mode (into compat 64 bit mode)
  mov 0xC00000080, %eax
  rdmsr
  orl 1 << 8, %eax
  wrmsr

   #turn paging back on
  mov %cr0, %eax
  mov 1 << 31, %eax
  mov cr0, eax
  jmp .done



.done:
  # Set up the stack pointer.
  mov $(stack + KSTACKSIZE), %esp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  mov $main, %eax
  jmp *%eax

.comm stack, KSTACKSIZE

