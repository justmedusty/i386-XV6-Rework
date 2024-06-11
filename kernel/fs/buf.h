struct buf {
  int flags;
  uint32 dev;
  uint32 blockno;
  struct sleeplock lock;
  uint32 refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uint8 data[BSIZE];
};
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk

