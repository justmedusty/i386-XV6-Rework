#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device
#define T_FIFO 4   // Named pipe

struct stat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint32 ino;    // Inode number
  short nlink; // Number of links to file
  uint32 size;   // Size of file in bytes
};
