// init: The initial user-level program

#include "../kernel/defs/types.h"
#include "../kernel/fs/stat.h"
#include "user.h"
#include "../kernel/fs/xfcntl.h"
#include "../kernel/syscall/syscall.h"

char *argv[] = { "login", 0 };

int
main(void)
{
  int pid, wpid;


  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf(1, "init: starting login shell\n");
    pid = fork();

    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("login", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
