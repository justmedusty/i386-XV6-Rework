// This file contains definitions for the
// x86 memory management unit (MMU).

// Eflags register
#define FL_IF           0x00000200      // Interrupt Enable

// Control Register flags
#define CR0_PE          0x00000001      // Protection Enable
#define CR0_WP          0x00010000      // Write Protect
#define CR0_PG          0x80000000      // Paging
#define CR0_PG_OFF     ~0x80000000      //turn paging off
#define CR4_PAE
#define CR4_PSE         0x00000010      // Page size extension

// various segment selectors.
#define SEG_KCODE 1  // kernel code
#define SEG_KDATA 2  // kernel data+stack
#define SEG_UCODE 3  // user code
#define SEG_UDATA 4  // user data+stack
#define SEG_TSS   5  // this process's task state

// cpu->gdt[NSEGS] holds the above segments.
#define NSEGS     6

#ifndef __ASSEMBLER__
// Segment Descriptor for 64-bit mode
struct segdesc {
    uint64 lim_15_0 : 16;  // Low bits of segment limit
    uint64 base_15_0 : 16; // Low bits of segment base address
    uint64 base_23_16 : 8; // Middle bits of segment base address
    uint64 type : 4;       // Segment type (see STS_ constants)
    uint64 s : 1;          // 0 = system, 1 = application
    uint64 dpl : 2;        // Descriptor Privilege Level
    uint64 p : 1;          // Present
    uint64 lim_19_16 : 4;  // High bits of segment limit
    uint64 avl : 1;        // Unused (available for software use)
    uint64 l : 1;          // 64-bit code segment (IA-64e mode only)
    uint64 db : 1;         // 0 = 16-bit segment, 1 = 64-bit segment (must be 0 for 64-bit)
    uint64 g : 1;          // Granularity: limit scaled by 4K when set
    uint64 base_31_24 : 8; // High bits of segment base address
    uint64 base_63_64 : 64; // Upper bits of segment base address
    uint64 reserved : 64; // Reserved
};

// Normal segment for 64-bit mode
#define SEG(type, base, lim, dpl) (struct segdesc)  \
{ ((lim) >> 12) & 0xffff, (uint64)(base) & 0xffff,    \
  ((uint64)(base) >> 16) & 0xff, type, 1, dpl, 1,     \
  (uint64)(lim) >> 28, 0, 1, 0, 1, (uint64)(base) >> 24, \
  (uint64)(base) >> 64, 0 }
#define SEG16(type, base, lim, dpl) (struct segdesc) \
{ (lim) & 0xffff, (uint64)(base) & 0xffff,            \
  ((uint64)(base) >> 16) & 0xff, type, 1, dpl, 1,     \
  (uint64)(lim) >> 16, 0, 0, 1, 0, (uint64)(base) >> 24, \
  (uint64)(base) >> 64, 0 }
#endif

#define DPL_USER    0x3     // User DPL

// Application segment type bits
#define STA_X       0x8     // Executable segment
#define STA_W       0x2     // Writeable (non-executable segments)
#define STA_R       0x2     // Readable (executable segments)

// System segment type bits
#define STS_T64A    0x9     // Available 32-bit TSS
#define STS_IG64    0xE     // 32-bit Interrupt Gate
#define STS_TG64    0xF     // 32-bit Trap Gate

// A virtual address 'la' has a three-part structure as follows:
//
// +-------9--------+----------9--------+---------9---------+---------9---------+---------12---------+
// |   P4D  Table   |     PUD Table     |      PMD Table    |      PTE Table    |   Offset within    |
// |      Index     |     Index         |        Index      |        Index      |         page       |
// +----------------+-------------------+-------------------+-------------------+--------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/


// page middle directory index
#define PGDX(va)         (((uint64)(va) >> PGDXSHIFT) & 0x1FF)
// page 4 directory index
#define P4DX(va)         (((uint64)(va) >> P4DXSHIFT) & 0x1FF)
// page upper directory index
#define PUDX(va)         (((uint64)(va) >> PUDXSHIFT) & 0x1FF)

// page middle directory index
#define PMDX(va)         (((uint64)(va) >> PMDXSHIFT) & 0x1FF)

// page table index
#define PTX(va)         (((uint64)(va) >> PTXSHIFT) & 0x1FF)

// construct virtual address from indexes (long mode) and offset
#define PGADDR(p4d,pud,pmd, t, o) ((uint64)((p4d << P4DXSHIFT | pud << PUDXSHIFT | (pmd) << PMDDXSHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NP4DENTRIES     512    // # directory entries per page middle directory
#define NPUDENTRIES     512    // # directory entries per page upper directory
#define NPMDENTRIES     512    // # directory entries per page middle directory
#define NPTENTRIES      512    // # PTEs per page table
#define PGSIZE          4096    // bytes mapped by a page

#define PTXSHIFT        12      // offset of PTX in a linear address
#define PMDXSHIFT       21     // offset of PMDX in a linear address
#define PUDXSHIFT       30     // offset of PUDX in a linear address
#define P4DXSHIFT       39     // offset of P4DX (or pgd if pae in a linear address

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// Page table/directory entry flags.
#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PS          0x080   // Page Size

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint64)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint64)(pte) &  0xFFF)

#ifndef __ASSEMBLER__
typedef uint64 pte_t;

// Task state segment format
struct taskstate {
  uint64 link;         // Old ts selector
  uint64 rsp0;         // Stack pointers and segment selectors
  uint16 ss0;        //   after an increase in privilege level
  uint16 padding1;
  uint64 *rsp1;
  uint16 ss1;
  uint16 padding2;
  uint64 *rsp2;
  uint16 ss2;
  uint16 padding3;
  void *cr3;         // Page directory base
  uint64 *rip;         // Saved state from last task switch
  uint64 eflags;
  uint64 rax;          // More saved state (registers)
  uint64 rcx;
  uint64 rdx;
  uint64 rbx;
  uint64 *rsp;
  uint64 *rbp;
  uint64 rsi;
  uint64 rdi;
  uint16 es;         // Even more saved state (segment selectors)
  uint16 padding4;
  uint16 cs;
  uint16 padding5;
  uint16 ss;
  uint16 padding6;
  uint16 ds;
  uint16 padding7;
  uint16 fs;
  uint16 padding8;
  uint16 gs;
  uint16 padding9;
  uint16 ldt;
  uint16 padding10;
  uint16 t;          // Trap on task switch
  uint16 iomb;       // I/O map base address
};

// Gate descriptors for trap and traps
struct gatedesc {
  uint64 off_15_0 : 16;   // low 16 bits of offset in segment
  uint64 cs : 16;         // code segment selector
  uint64 args : 5;        // # args, 0 for interrupt/trap gates
  uint64 rsv1 : 3;        // reserved(should be zero I guess)
  uint64 type : 4;        // type(STS_{IG64,TG64})
  uint64 s : 1;           // must be 0 (system)
  uint64 dpl : 2;         // descriptor(meaning new) privilege level
  uint64 p : 1;           // Present
  uint64 off_31_16 : 16;  // high bits of offset in segment
};

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, d)                \
{                                                         \
  (gate).off_15_0 = (uint64)(off) & 0xffff;                \
  (gate).cs = (sel);                                      \
  (gate).args = 0;                                        \
  (gate).rsv1 = 0;                                        \
  (gate).type = (istrap) ? STS : STS_IG64;           \
  (gate).s = 0;                                           \
  (gate).dpl = (d);                                       \
  (gate).p = 1;                                           \
  (gate).off_31_16 = (uint64)(off) >> 16;                  \
}

#endif
