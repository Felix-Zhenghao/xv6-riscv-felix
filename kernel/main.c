#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0; // tell compiler the variable can be accessed by multiple threads

// start() jumps here in supervisor mode on all CPUs.
// each core will be begin executing this function in parallel
void
main()
{
  if(cpuid() == 0){
    // cpuid will look at the tp register and return the core id, so core 0 will execute this block
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode table
    fileinit();      // file table
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    __sync_synchronize(); // a compiler magic to tell compiler to make sure all previous codes are executed before running codes after this line
    started = 1; // finish initialization, so other cores can start executing
  } else {
    // all other cores (id != 0) will execute this block
    while(started == 0) // other threads do nothing if thread 0 does not finish initialization
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler(); // after all cores are initialized, each core will call the scheduler function so that all cores can start executing processes 
}

/*
kvminithart(), trapinithart(), plicinithart() are core-level initialization functions and are executed by each core.
core with id==0 do more fundamental initialization work than other cores.
*/



