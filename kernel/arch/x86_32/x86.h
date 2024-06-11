// Routines to let C code use special x86 instructions.

// Reads a byte from the specified I/O port.
static inline uint8 inb(uint16 port)
{
    uint8 data;
    asm volatile("in %1,%0" : "=a" (data) : "d" (port));
    return data;
}

// Reads a sequence of 32-bit values from the specified I/O port into memory.
static inline void insl(int port, void *addr, int cnt)
{
    asm volatile("cld; rep insl" :
            "=D" (addr), "=c" (cnt) :
            "d" (port), "0" (addr), "1" (cnt) :
            "memory", "cc");
}

// Writes a byte to the specified I/O port.
static inline void outb(uint16 port, uint8 data)
{
    asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

// Writes a 16-bit value to the specified I/O port.
static inline void outw(uint16 port, uint16 data)
{
    asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

// Writes a sequence of 32-bit values from memory to the specified I/O port.
static inline void outsl(int port, const void *addr, int cnt)
{
    asm volatile("cld; rep outsl" :
            "=S" (addr), "=c" (cnt) :
            "d" (port), "0" (addr), "1" (cnt) :
            "cc");
}

// Writes a byte value repeatedly to a memory location.
static inline void stosb(void *addr, int data, int cnt)
{
    asm volatile("cld; rep stosb" :
            "=D" (addr), "=c" (cnt) :
            "0" (addr), "1" (cnt), "a" (data) :
            "memory", "cc");
}

// Writes a 32-bit value repeatedly to a memory location.
static inline void stosl(void *addr, int data, int cnt)
{
    asm volatile("cld; rep stosl" :
            "=D" (addr), "=c" (cnt) :
            "0" (addr), "1" (cnt), "a" (data) :
            "memory", "cc");
}

// Loads the Global Descriptor Table (GDT) register.
static inline void lgdt(struct segdesc *p, int size)
{
    volatile uint16 pd[3];
    pd[0] = size-1;
    pd[1] = (uint32)p;
    pd[2] = (uint32)p >> 16;
    asm volatile("lgdt (%0)" : : "r" (pd));
}

// Loads the Interrupt Descriptor Table (IDT) register.
static inline void lidt(struct gatedesc *p, int size)
{
    volatile uint16 pd[3];
    pd[0] = size-1;
    pd[1] = (uint32)p;
    pd[2] = (uint32)p >> 16;
    asm volatile("lidt (%0)" : : "r" (pd));
}

// Loads the Task Register (TR).
static inline void ltr(uint16 sel)
{
    asm volatile("ltr %0" : : "r" (sel));
}

// Reads the EFLAGS register.
static inline uint32 readeflags(void)
{
    uint32 eflags;
    asm volatile("pushfl; popl %0" : "=r" (eflags));
    return eflags;
}

// Loads a 16-bit value into the GS segment register.
static inline void loadgs(uint16 v)
{
    asm volatile("movw %0, %%gs" : : "r" (v));
}

// Disables trap.
static inline void cli(void)
{
    asm volatile("cli");
}

// Enables trap.
static inline void sti(void)
{
    asm volatile("sti");
}

// Atomic exchange of a value.
static inline uint32 xchg(volatile uint32 *addr, uint32 newval)
{
    uint32 result;
    asm volatile("lock; xchgl %0, %1" :
            "+m" (*addr), "=a" (result) :
            "1" (newval) :
            "cc");
    return result;
}

// Reads the CR2 register.
static inline uint32 rcr2(void)
{
    uint32 val;
    asm volatile("movl %%cr2,%0" : "=r" (val));
    return val;
}

// Loads a value into the CR3 register.
static inline void lcr3(uint32 val)
{
    asm volatile("movl %0,%%cr3" : : "r" (val));
}

//PAGEBREAK: 36
// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe {
  // registers as pushed by pusha
  uint32 edi;
  uint32 esi;
  uint32 ebp;
  uint32 oesp;      // useless & ignored
  uint32 ebx;
  uint32 edx;
  uint32 ecx;
  uint32 eax;

  // rest of trap frame
  uint16 gs;
  uint16 padding1;
  uint16 fs;
  uint16 padding2;
  uint16 es;
  uint16 padding3;
  uint16 ds;
  uint16 padding4;
  uint32 trapno;

  // below here defined by x86 hardware
  uint32 err;
  uint32 eip;
  uint16 cs;
  uint16 padding5;
  uint32 eflags;

  // below here only when crossing rings, such as from user to kernel
  uint32 esp;
  uint16 ss;
  uint16 padding6;
};
