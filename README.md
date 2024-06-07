This is my rework of the x86 xv6 operating system.


IMPORTANT NOTE: I add changes in many commits, and it may be broken between these implementations, if you are trying to run this yourself find a commit with a message prefix of FUNCTIONAL. Going forward I will mark commits as FUNCTIONAL if everything is working properly so you can find a commit that works if you wish to run it.

If a commit message is preceded by FUNCTIONAL: then this commit is functional and you can build and run it, if it is not there it means I am in between changes and it is either partially functional or not functional at all


Changes made so far:

  - Sched queue based on process priorities, differing from typical unix in that higher number eq higher prio. 
    Insertions into the sched queue are done by placing the highest prio first on the runqeue and as the kernel walks
    down the queue it checks the process to be enqueued against the process in this spot in the queue.

  - Added basic function to do cpu usage averaging to dynamically set time quanta for processes based on a the mean process life in clock cycles 
    
  - Added per-cpu runqeueus so each cpu will have its own runqueue, will also soon implement rebalancing alongside this

  - Added basic signals, can be seen in signal.h. Signals can be masked and ignored via the sigignore system call (non fatal signals only)
    signal handlers not properly implemented yet will get to this later.

  - Added mounting of secondary filesystems, can be mounted on any directory in your main filesystem. Can traverse across the mount point and use commands
    across the mountpoint

  - Added multi-disk support in the ide driver. It only supports 2 disks right now so I need to clean it up at some point and allow the maximum disks and just do a probe check
    on boot to check what controllers are there and if any disks are attatched.

  - Added preemption of processes if the time quantum is exceeded and a higher prio process is waiting, i added the special PREEMPTED process state so that I can ensure fairness
    and allow a preempted process to execute again later even if it is low prio.

  - Added nonblocking lock specifically for mounting. It is just a lock that returns immediately if locked instead of spinning or sleeping.

  - Added login shell, only supports 1 pair of credentials located in the passwd file in the users directory

  - Freemem function which spits out the # of pages allocated to all user processes (very useful for tracking memory usage when you play around with memory management)

  - Sig command which uses the sig system call to send a signal to a process

  - Mountfs and umountfs user programs which can be run from the shell, allows mounting of the secondary disk on the secondary ata controller, usage
    is mountfs dev dir (ie mountfs 2 /dir) It does not make new nodes so you need to create the directory to mount onto with mkdir. Unmount works as umountfs mountdir (ie umountfs /dir).


How to build (linux):

- Download QEMU & the source
- Ensure you get the source from a commit that is preceded by FUNCTIONAL
- Navigate to /users in the source tree
- Enter 'Make qemu' in your terminal (This builds vectors.S from the pl file, both hard disk images, and the kernel, and starts qemu)
- You need to update the passwd file in /users if you want to use any other user/pass combo (only supports 1 set of credentials)