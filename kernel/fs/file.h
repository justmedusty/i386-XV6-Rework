struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE , FD_FIFO} type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint32 off;
};


// in-memory copy of an inode
struct inode {
  uint32 dev;           // Device number
  uint32 inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?
  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint32 size;
  int is_mount_point; // 0 for no 1 for yes
  uint32 addrs[NDIRECT+1];
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
