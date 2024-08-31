// Mutual exclusion spin locks.

/*
Why we need to aviod interruptions when acquiring a lock?

- What is interrupt?
  - It is a way to communicate with the processor to stop what it is doing and do something else.
  - The original task can be resumed after the interrupt is handled.

- Why we need to avoid interruptions when acquiring a lock?
  - If the original task is interrupted while acquiring a lock and got the lock;
  - And the new task is trying to acquire the same lock;
  - The new task will be blocked because the lock is already held by the original task;
  - The original task will be blocked because it is interrupted and cannot release the lock;
  - This is a deadlock.

- That sounds unlikely to happen, why we need to avoid it?
  - In OS, if something can happen, it will happen. No matter how unlikely it is.

- Solution:
  - Disable interruptions when acquiring a lock.
  - Enable interruptions when releasing a lock.
*/

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0; // lock is not held so set to 0
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void
acquire(struct spinlock *lk)
{
  push_off(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire");

  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  // Atomic swap is a single instruction that reads the value of a memory location, modifies it, and returns the original value
  //   a5 = 1
  //   s1 = &lk->locked
  //   amoswap.w.aq a5, a5, (s1)
  // The following code will stay in a loop until the lock is not held by another CPU (so it can be acquired)
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0) // pass in a pointer to the field want to update and the new value and check whether the old value is 0 or not
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  // Once acquire the lock, record the CPU that holds the lock.
  // Record info about lock acquisition for holding() and debugging.
  lk->cpu = mycpu(); // return a pointer to the structure of the core that's executing this code
}

// Release the lock.
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  // On RISC-V, sync_lock_release turns into an atomic swap:
  //   s1 = &lk->locked
  //   amoswap.w zero, zero, (s1)
  __sync_lock_release(&lk->locked);

  pop_off(); // a dual partner of push_off() which enables interrupts
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

/*
Why we need noff and intena rather than directly use intr_off() and intr_on()?
- noff let us know how many times we call push_off(). Because we may have multiple critical area and multiple `acquire`, we don't want `release` to permanently enable the interrupt once and for all.
- intena let us know whether the interrupt is enabled before we call push_off() for the first time. If interupt is not enabled before first push_off(), we don't want to enable it after pop_off().
*/

void
push_off(void)
{
  int old = intr_get(); // get the previous interrupt state

  intr_off(); // turn off the interrupt
  if(mycpu()->noff == 0) // if interrupt is enabled before push_off()
    mycpu()->intena = old;
  mycpu()->noff += 1;
}

void
pop_off(void)
{
  struct cpu *c = mycpu();
  if(intr_get()) // if interrupt is enabled and we call pop_off(), it will cause a panic
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if(c->noff == 0 && c->intena) // if interrupt is not enabled before push_off(), we still can't enable it after pop_off()
    intr_on();
}
