// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held? If not, 0, if held, 1.

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

/*
A Spinlock should not be hold for a long time.

Because as we can see in spinlock.c implementation, when a lock is acquired, it will just loop,
  waiting for the lock to be released. This will waste CPU cycles.

Spinlock is used to protect critical sections that are very short and can be executed quickly:
  acquire()
  ... // critical section, only one thread can execute this at a time
  release()
*/