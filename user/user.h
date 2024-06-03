#include "../kernel/defs/types.h"
struct stat;
struct rtcdate;

//Errors defined here as well
#define ESIG                    1000000000    //Bad signal || no such signal
#define ENOPROC                 1000000001    // No proc of this pid found

#define ENOMOUNT                2 //unmount spot not a mountpoint
#define EMOUNTNTDIR             3 //mount point not directory
#define EMNTPNTNOTFOUND         4 //mount point not found
#define EMOUNTPNTLOCKED         5 //mount lock is taken
#define EMOUNTPOINTBUSY         6 //ref count too high
#define ECANNOTMOUNTONROOT      7 //cannot mount on dev 1 root
#define EMOUNTROOTNOTFOUND      8 //mount root not found , could be no filesystem on that device
#define ENODEV                  9 //dev not present
#define EDEVOOR                 10 //out of range
#define ECANNOTMOUNTONMAIN      11 // cannot mount main disk / drive (why would you mount the same device onto itself unless you like recursive mounting for some reason)
// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int freemem(void);
int sig(int, int);
void sighandler(void (*));
void sigignore(int,int);
void changeconsmode(int);
int mount(int,char*);
int umount(char*);


void stack_overflow(int x);
void stack_overflow2(int x);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

