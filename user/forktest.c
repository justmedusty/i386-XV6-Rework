// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include "types.h"
#include "stat.h"
#include "user.h"

#define N  1000

void
printf2(int fd, const char *s, ...)
{
  write(fd, s, strlen(s));
}

void
forktest(void)
{
  int n, pid;

  printf2(1, "fork test\n");

  for(n=0; n<N; n++){
    pid = fork();
    if(pid < 0)
      break;
    if(pid == 0)
      exit();
  }

  if(n == N){
    printf2(1, "fork claimed to work N times!\n", N);
    exit();
  }
/*
 * Since there are no more zombies and procs are wiped upon exit, this can be commented out
 */

  for(; n > 0; n--){
    if(wait() < 0){
      printf2(1, "wait stopped early\n");
     exit();
    }
  }

  if(wait() != -1){
    printf2(1, "wait got too many\n");
    exit();
  }

  printf2(1, "fork test OK\n");
}

int
main(void)
{
  forktest();
  exit();
}
