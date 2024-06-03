OBJS = \
	kernel/fs/bio.o\
	kernel/dev/console.o\
	kernel/exec.o\
	kernel/fs/file.o\
	kernel/fs/fs.o\
	kernel/drivers/ide.o\
	kernel/arch/x86_32/cpu/ioapic.o\
	kernel/mm/kalloc.o\
	kernel/drivers/kbd.o\
	kernel/arch/x86_32/cpu/lapic.o\
	kernel/fs/log.o\
	kernel/main.o\
	kernel/fs/mount.o\
	kernel/arch/x86_32/mp/mp.o\
	kernel/lock/nonblockinglock.o\
	kernel/arch/x86_32/cpu/picirq.o\
	kernel/ipc/pipe.o\
	kernel/sched/proc.o\
	kernel/lock/sleeplock.o\
	kernel/lock/spinlock.o\
	kernel/mm/string.o\
	kernel/arch/x86_32/swtch.o\
	kernel/syscall/syscall.o\
	kernel/syscall/sysfile.o\
	kernel/syscall/sysproc.o\
	kernel/arch/x86_32/trapasm.o\
	kernel/interrupts/trap.o\
	kernel/drivers/uart.o\
	kernel/scripts/vectors.o\
	kernel/mm/vm.o \

# Cross-compiling (e.g., on Mac OS X)
#TOOLPREFIX = i386-jos-elf

# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX = elf32-i386

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your TOOLPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	elif which qemu-system-i386 > /dev/null; \
	then echo qemu-system-i386; exit; \
	elif which qemu-system-x86_64 > /dev/null; \
	then echo qemu-system-x86_64; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -mno-sse -m32 -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
# FreeBSD ld wants ``elf_i386_fbsd''
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

xv6.img: bootblock xkernel
	dd if=/dev/zero of=xv6.img count=10000
	dd if=bootblock of=xv6.img conv=notrunc
	dd if=xkernel of=xv6.img seek=1 conv=notrunc

#xv6memfs.img: bootblock kernelmemfs
#	dd if=/dev/zero of=xv6memfs.img count=10000
#	dd if=bootblock of=xv6memfs.img conv=notrunc
#	dd if=kernelmemfs of=xv6memfs.img seek=1 conv=notrunc

bootblock: kernel/arch/x86_32/boot/bootasm.S kernel/boot/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I. -c kernel/boot/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c kernel/arch/x86_32/boot/bootasm.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
	./kernel/scripts/sign.pl bootblock

entryother: kernel/arch/x86_32/boot/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c  kernel/arch/x86_32/boot/entryother.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
	$(OBJDUMP) -S bootblockother.o > entryother.asm

initcode: kernel/arch/x86_32/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c kernel/arch/x86_32/initcode.S
	$(OBJCOPY) --remove-section .note.gnu.property initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

kernel: $(OBJS) kernel/arch/x86_32/boot/entry.o
	$(LD) $(LDFLAGS) -T kernel/scripts/kernel.ld -o xkernel kernel/arch/x86_32/boot/entry.o $(OBJS) -b binary initcode entryother
	$(OBJDUMP) -S xkernel > kernel.asm
	$(OBJDUMP) -t xkernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

# kernelmemfs is a copy of kernel that maintains the
# disk image in memory instead of writing to a disk.
# This is not so useful for testing persistent storage or
# exploring disk buffering implementations, but it is
# great for testing the kernel on real hardware without
# needing a scratch disk.
#MEMFSOBJS = $(filter-out ide.o,$(OBJS)) #memide.o
#kernelmemfs: $(MEMFSOBJS) entry.o entryother initcode kernel.ld #fs.img secondaryfs.img
#	$(LD) $(LDFLAGS) -T kernel.ld -o kernelmemfs entry.o  $(MEMFSOBJS) -b binary initcode entryother #fs.img secondaryfs.img
#	$(OBJDUMP) -S kernelmemfs > kernelmemfs.asm
#	$(OBJDUMP) -t kernelmemfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernelmemfs.sym

tags: $(OBJS) kernel/boot _init
	etags *.S *.c

vectors.S: kernel/scripts/vectors.pl
	./kernel/scripts/vectors.pl > kernel/scripts/vectors.S

ULIB = user/ulib.o kernel/syscall/usys.o user/printf.o user/umalloc.o

_%: %/user/*.o $(ULIB)
	$(OBJCOPY) --remove-section .note.gnu.property user/ulib.o
	$(LD) $(LDFLAGS) -N -e kernel/main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

_forktest: user/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e kernel/main -Ttext 0 -o user/_forktest user/forktest.o user/ulib.o kernel/syscall/usys.o
	$(OBJDUMP) -S user/_forktest > user/forktest.asm

mkfs: kernel/fs/mkfs.c kernel/fs/fs.h
	gcc -Werror -Wall -o kernel/fs/mkfs kernel/fs/mkfs.c
# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	user/_cat\
	user/_echo\
	user/_forktest\
	user/_grep\
	user/_init\
	user/_kill\
	user/_ln\
	user/_ls\
	user/_mkdir\
	user/_rm\
	user/_sh\
	user/_stressfs\
	user/_usertests\
	user/_wc\
	user/_zombie\
	user/_freemem\
	user/_sig\
	user/_login\
	user/_mountfs\
	user/_umountfs\

fs.img: kernel/fs/mkfs README files/passwd files/largefile $(UPROGS)
	./kernel/fs/mkfs  fs.img README files/passwd files/largefile $(UPROGS)

secondaryfs.img: mkfs README files/largefile
	./kernel/fs/mkfs secondaryfs.img README files/largefile user/_ls user/_cat
-include *.d

clean:
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*.o *.d *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out xv6.img fs.img kernelmemfs \
	xv6memfs.img mkfs .gdbinit secondaryfs.img \
	kernel/*/*.o user/*.sym user/*.asm kernel/*/*.d user/*.o user/*.d kernel/*.o kernel/*.d kernel/*/*/*.o kernel/*/*/*/*.o kernel/*/*/*/*.d kernel/*/*/*/*.asm kernel/*/*/*.d \
	$(UPROGS)

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 4
endif
QEMUOPTS =     -drive file=xv6.img,media=disk,format=raw,bus=0,unit=0 \
               -drive file=fs.img,media=disk,format=raw,bus=0,unit=1 \
               -drive file=secondaryfs.img,media=disk,format=raw,bus=1,unit=0 \
               -smp $(CPUS) -m 512 $(QEMUEXTRA) \
               -monitor stdio

drives: fs.img secondaryfs.img

qemu: xv6.img
	$(QEMU) -serial mon:vc $(QEMUOPTS)

#qemu-memfs: xv6memfs.img
#	$(QEMU) -drive file=xv6memfs.img,index=0,media=disk,format=raw -smp $(CPUS) -m 256

qemu-nox: fs.img secondaryfs.img xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: files/.gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb: fs.img secondaryfs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)
