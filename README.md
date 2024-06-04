This is my rework of the x86 xv6 operating system.

Changes made so far:

  - Sched queue based on processor priorities, differing from typical unix in that higher number eq higher prio. 
    Insertions into the sched queue are done by placing the highest prio first on the runqeue and as the kernel walks
    down the queue it checks the process to be enqueued against the process in this spot in the queue.

  - Added basic signals, can be seen in signal.h. Signals can be masked and ignored via the sigignore system call (non fatal signals only)
    signal handlers not properly implemented yet will get to this later.

  - Added mounting of secondary filesystems, can be mounted on any directory in your main filesystem. Can traverse across the mount point and use commands
    across the mountpoint

  - Added multi-disk support in the ide driver. It only supports 2 disks right now so I need to clean it up at some point and allow the maximum disks and just do a probe check
    on boot to check what controllers are there and if any disks are attatched.

  - Added preemption of processes if the time quantum is exceeded and a higher prio process is waiting, i added the special PREEMPTED process state so that I can ensure fairness
    and allow a preempted process to execute again later even if it is low prio.

  - Added nonblocking lock specifically for mounting. It is just a lock that returns immediately if locked instead of spinning or sleeping.