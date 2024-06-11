// This file contains definitions for the
// x86 memory management unit (MMU).

// Eflags register
#define FL_IF           0x00000200      // Interrupt Enable

// Control Register flags
#define CR0_PE          0x00000001      // Protection Enable
#define CR0_WP          0x00010000      // Write Protect
#define CR0_PG          0x80000000      // Paging

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
// Segment Descriptor
struct segdesc {
  uint32 lim_15_0 : 16;  // Low bits of segment limit
  uint32 base_15_0 : 16; // Low bits of segment base address
  uint32 base_23_16 : 8; // Middle bits of segment base address
  uint32 type : 4;       // Segment type (see STS_ constants)
  uint32 s : 1;          // 0 = system, 1 = application
  uint32 dpl : 2;        // Descriptor Privilege Level
  uint32 p : 1;          // Present
  uint32 lim_19_16 : 4;  // High bits of segment limit
  uint32 avl : 1;        // Unused (available for software use)
  uint32 rsv1 : 1;       // Reserved
  uint32 db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
  uint32 g : 1;          // Granularity: limit scaled by 4K when set
  uint32 base_31_24 : 8; // High bits of segment base address
};

// Normal segment
#define SEG(type, base, lim, dpl) (struct segdesc)    \
{ ((lim) >> 12) & 0xffff, (uint32)(base) & 0xffff,      \
  ((uint32)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
  (uint32)(lim) >> 28, 0, 0, 1, 1, (uint32)(base) >> 24 }
#define SEG16(type, base, lim, dpl) (struct segdesc)  \
{ (lim) & 0xffff, (uint32)(base) & 0xffff,              \
  ((uint32)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
  (uint32)(lim) >> 16, 0, 0, 1, 0, (uint32)(base) >> 24 }
#endif

#define DPL_USER    0x3     // User DPL

// Application segment type bits
#define STA_X       0x8     // Executable segment
#define STA_W       0x2     // Writeable (non-executable segments)
#define STA_R       0x2     // Readable (executable segments)

// System segment type bits
#define STS_T32A    0x9     // Available 32-bit TSS
#define STS_IG32    0xE     // 32-bit Interrupt Gate
#define STS_TG32    0xF     // 32-bit Trap Gate

// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/

// page directory index
#define PDX(va)         (((uint32)(va) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(va)         (((uint32)(va) >> PTXSHIFT) & 0x3FF)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uint32)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NPDENTRIES      1024    // # directory entries per page directory
#define NPTENTRIES      1024    // # PTEs per page table
#define PGSIZE          4096    // bytes mapped by a page

#define PTXSHIFT        12      // offset of PTX in a linear address
#define PDXSHIFT        22      // offset of PDX in a linear address

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// Page table/directory entry flags.
#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PS          0x080   // Page Size

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint32)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint32)(pte) &  0xFFF)

#ifndef __ASSEMBLER__
typedef uint32 pte_t;

// Task state segment format
struct taskstate {
  uint32 link;         // Old ts selector
  uint32 esp0;         // Stack pointers and segment selectors
  uint16 ss0;        //   after an increase in privilege level
  uint16 padding1;
  uint32 *esp1;
  uint16 ss1;
  uint16 padding2;
  uint32 *esp2;
  uint16 ss2;
  uint16 padding3;
  void *cr3;         // Page directory base
  uint32 *eip;         // Saved state from last task switch
  uint32 eflags;
  uint32 eax;          // More saved state (registers)
  uint32 ecx;
  uint32 edx;
  uint32 ebx;
  uint32 *esp;
  uint32 *ebp;
  uint32 esi;
  uint32 edi;
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
  uint32 off_15_0 : 16;   // low 16 bits of offset in segment
  uint32 cs : 16;         // code segment selector
  uint32 args : 5;        // # args, 0 for interrupt/trap gates
  uint32 rsv1 : 3;        // reserved(should be zero I guess)
  uint32 type : 4;        // type(STS_{IG32,TG32})
  uint32 s : 1;           // must be 0 (system)
  uint32 dpl : 2;         // descriptor(meaning new) privilege level
  uint32 p : 1;           // Present
  uint32 off_31_16 : 16;  // high bits of offset in segment
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
  (gate).off_15_0 = (uint32)(off) & 0xffff;                \
  (gate).cs = (sel);                                      \
  (gate).args = 0;                                        \
  (gate).rsv1 = 0;                                        \
  (gate).type = (istrap) ? STS_TG32 : STS_IG32;           \
  (gate).s = 0;                                           \
  (gate).dpl = (d);                                       \
  (gate).p = 1;                                           \
  (gate).off_31_16 = (uint32)(off) >> 16;                  \
}

#endif
