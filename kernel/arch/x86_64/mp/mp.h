// See MultiProcessor Specification Version 1.[14]

struct mp {             // floating pointer
  uint8 signature[4];           // "_MP_"
  void *physaddr;               // phys addr of MP config table
  uint8 length;                 // 1
  uint8 specrev;                // [14]
  uint8 checksum;               // all bytes must add up to 0
  uint8 type;                   // MP system config type
  uint8 imcrp;
  uint8 reserved[3];
};

struct mpconf {         // configuration table header
  uint8 signature[4];           // "PCMP"
  uint16 length;                // total table length
  uint8 version;                // [14]
  uint8 checksum;               // all bytes must add up to 0
  uint8 product[20];            // product id
  uint64 *oemtable;               // OEM table pointer
  uint16 oemlength;             // OEM table length
  uint16 entry;                 // entry count
  uint64 *lapicaddr;              // address of local APIC
  uint16 xlength;               // extended table length
  uint8 xchecksum;              // extended table checksum
  uint8 reserved;
};

struct mpproc {         // processor table entry
  uint8 type;                   // entry type (0)
  uint8 apicid;                 // local APIC id
  uint8 version;                // local APIC verison
  uint8 flags;                  // CPU flags
    #define MPBOOT 0x02           // This proc is the bootstrap processor.
  uint8 signature[4];           // CPU signature
  uint32 feature;                 // feature flags from CPUID instruction
  uint8 reserved[8];
};

struct mpioapic {       // I/O APIC table entry
  uint8 type;                   // entry type (2)
  uint8 apicno;                 // I/O APIC id
  uint8 version;                // I/O APIC version
  uint8 flags;                  // I/O APIC flags
  uint64 *addr;                  // I/O APIC address
};

// Table entry types
#define MPPROC    0x00  // One per processor
#define MPBUS     0x01  // One per bus
#define MPIOAPIC  0x02  // One per I/O APIC
#define MPIOINTR  0x03  // One per bus interrupt source
#define MPLINTR   0x04  // One per system interrupt source


int num_cpus();
//PAGEBREAK!
// Blank page.
