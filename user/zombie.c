// Create a zombie process that
// must be reparented at exit.

#include "../kernel/defs/types.h"
#include "../kernel/fs/stat.h"
#include "user.h"

int
main(void)
{
  if(fork() > 0)
    sleep(5);  // Let child exit before parent.
  exit();
}
