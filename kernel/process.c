/*
 * Utility functions for process management. 
 *
 * Note: in Lab1, only one process (i.e., our user application) exists. Therefore, 
 * PKE OS at this stage will set "current" to the loaded user application, and also
 * switch to the old "current" process after trap handling.
 */

#include "riscv.h"
#include "strap.h"
#include "config.h"
#include "process.h"
#include "elf.h"
#include "string.h"
#include "vmm.h"
#include "pmm.h"
#include "memlayout.h"
#include "sched.h"
#include "spike_interface/spike_utils.h"
#include "sync_utils.h"
volatile int count_3=1;
//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe *, uint64 satp);

// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point
// of S-mode trap vector).
extern char trap_sec_start[];

// process pool. added @lab3_1
process procs[NPROC];

// current points to the currently running user-mode application.
process* current [2];
uint64 g_ufree_page = USER_FREE_ADDRESS_START;
//
// switch to a user-mode process
//
void  switch_to(process* proc) {
  assert(proc);
  uint64 tp=read_tp();
  current[tp] = proc;
  //sprint("111\n");
  // write the smode_trap_vector (64-bit func. address) defined in kernel/strap_vector.S
  // to the stvec privilege register, such that trap handler pointed by smode_trap_vector
  // will be triggered when an interrupt occurs in S mode.
  write_csr(stvec, (uint64)smode_trap_vector);

  // set up trapframe values (in process structure) that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;      // process's kernel stack
  proc->trapframe->kernel_satp = read_csr(satp);  // kernel page table
  //sprint("stap:%x\n",proc->trapframe->kernel_satp );
  //sprint("pagetable:%x\n",proc->pagetable);
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // SSTATUS_SPP and SSTATUS_SPIE are defined in kernel/riscv.h
  // set S Previous Privilege mode (the SSTATUS_SPP bit in sstatus register) to User mode.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  // write x back to 'sstatus' register to enable interrupts, and sret destination mode.
  write_csr(sstatus, x);

  // set S Exception Program Counter (sepc register) to the elf entry pc.
  write_csr(sepc, proc->trapframe->epc);

  // make user page table. macro MAKE_SATP is defined in kernel/riscv.h. added @lab2_1
  uint64 user_satp = MAKE_SATP(proc->pagetable);

  // return_to_user() is defined in kernel/strap_vector.S. switch to user mode with sret.
  // note, return_to_user takes two parameters @ and after lab2_1.
  //sprint("111\n");
  //sprint("sepc:%x\n", proc->trapframe->epc);
  //sprint("%x\n",user_satp);
  //uint64 pa=*(uint64 *)user_va_to_pa(proc->pagetable,(void *)(0x116f8));
  //sprint("%x\n",pa);
  //sprint("switch sp%x\n",current[tp]->trapframe->regs.sp);
  //sprint("11\n");
  return_to_user(proc->trapframe, user_satp);
  
}

//
// initialize process pool (the procs[] array). added @lab3_1
//
void init_proc_pool() {
  memset( procs, 0, sizeof(process)*NPROC );
  uint64 tp=read_tp();
  for (int i = 0; i < NPROC; ++i) {
    procs[i].status = FREE;
    procs[i].pid = i;
  }
}

//
// allocate an empty process, init its vm space. returns the pointer to
// process strcuture. added @lab3_1
//
process* alloc_process() {
  // locate the first usable process structure
  uint64 tp=read_tp();
  //sprint("%d\n",tp);
  int i;
  sem_P(&count_3);
  for( i=0; i<NPROC; i++ )
    if( procs[i].status == FREE ) break;

  if( i>=NPROC ){
    panic( "cannot find any free process structure.\n" );
    return 0;
  }

  // init proc[i]'s vm space
  procs[i].trapframe = (trapframe *)alloc_page();  //trapframe, used to save context
  memset(procs[i].trapframe, 0, sizeof(trapframe));

  // page directory
  procs[i].pagetable = (pagetable_t)alloc_page();
  
  memset((void *)procs[i].pagetable, 0, PGSIZE);

  procs[i].kstack = (uint64)alloc_page() + PGSIZE;   //user kernel stack top
  uint64 user_stack = (uint64)alloc_page();       //phisical address of user stack bottom
  procs[i].trapframe->regs.sp = USER_STACK_TOP;  //virtual address of user stack top

  // allocates a page to record memory regions (segments)
  procs[i].mapped_info = (mapped_region*)alloc_page();
  memset( procs[i].mapped_info, 0, PGSIZE );

  // map user stack in userspace
  user_vm_map((pagetable_t)procs[i].pagetable, USER_STACK_TOP - PGSIZE, PGSIZE,
    user_stack, prot_to_type(PROT_WRITE | PROT_READ, 1));
  procs[i].mapped_info[STACK_SEGMENT].va = USER_STACK_TOP - PGSIZE;
  procs[i].mapped_info[STACK_SEGMENT].npages = 1;
  procs[i].mapped_info[STACK_SEGMENT].seg_type = STACK_SEGMENT;

  // map trapframe in user space (direct mapping as in kernel space).
  user_vm_map((pagetable_t)procs[i].pagetable, (uint64)procs[i].trapframe, PGSIZE,
    (uint64)procs[i].trapframe, prot_to_type(PROT_WRITE | PROT_READ, 0));
  procs[i].mapped_info[CONTEXT_SEGMENT].va = (uint64)procs[i].trapframe;
  procs[i].mapped_info[CONTEXT_SEGMENT].npages = 1;
  procs[i].mapped_info[CONTEXT_SEGMENT].seg_type = CONTEXT_SEGMENT;

  // map S-mode trap vector section in user space (direct mapping as in kernel space)
  // we assume that the size of usertrap.S is smaller than a page.
  user_vm_map((pagetable_t)procs[i].pagetable, (uint64)trap_sec_start, PGSIZE,
    (uint64)trap_sec_start, prot_to_type(PROT_READ | PROT_EXEC, 0));
  procs[i].mapped_info[SYSTEM_SEGMENT].va = (uint64)trap_sec_start;
  procs[i].mapped_info[SYSTEM_SEGMENT].npages = 1;
  procs[i].mapped_info[SYSTEM_SEGMENT].seg_type = SYSTEM_SEGMENT;

  // sprint("in alloc_proc. user frame 0x%lx, user stack 0x%lx, user kstack 0x%lx \n",
  //   procs[i].trapframe, procs[i].trapframe->regs.sp, procs[i].kstack);

  // initialize the process's heap manager
  procs[i].user_heap.heap_top = USER_FREE_ADDRESS_START;
  procs[i].user_heap.heap_bottom = USER_FREE_ADDRESS_START;
  procs[i].user_heap.free_pages_count = 0;

  // map user heap in userspace
  procs[i].mapped_info[HEAP_SEGMENT].va = USER_FREE_ADDRESS_START;
  procs[i].mapped_info[HEAP_SEGMENT].npages = 0;  // no pages are mapped to heap yet.
  procs[i].mapped_info[HEAP_SEGMENT].seg_type = HEAP_SEGMENT;

  procs[i].total_mapped_region = 4;

  // initialize files_struct
  procs[i].pfiles = init_proc_file_management();
  //sprint("in alloc_proc. build proc_file_management successfully.\n");
  //sprint(" procs[i].kstack:%x\n",procs[i].kstack);
  // return after initialization.
  //sprint("alloc pagetable:%x\n",procs[i].pagetable);
   procs[i].status = USED;
  sem_V(&count_3);
  //sprint("proc :sp%x\n",procs[i].trapframe->regs.sp);
  return &procs[i];
}

//
// reclaim a process. added @lab3_1
//
int free_process( process* proc ) {
  // we set the status to ZOMBIE, but cannot destruct its vm space immediately.
  // since proc can be current process, and its user kernel stack is currently in use!
  // but for proxy kernel, it (memory leaking) may NOT be a really serious issue,
  // as it is different from regular OS, which needs to run 7x24.
  uint64 tp=read_tp();
  for( int i=0; i<NPROC; i++ )
    if(procs[i].pid==proc->pid)
    {
      procs[i].status=ZOMBIE;
      //sprint("111\n");
    }
   
  proc->status = ZOMBIE;

  return 0;
}
int free_p( process* proc ) {
  // we set the status to ZOMBIE, but cannot destruct its vm space immediately.
  // since proc can be current process, and its user kernel stack is currently in use!
  // but for proxy kernel, it (memory leaking) may NOT be a really serious issue,
  // as it is different from regular OS, which needs to run 7x24.
  uint64 tp=read_tp();
  for( int i=0; i<NPROC; i++ )
    if(procs[i].pid==proc->pid)
    {
      procs[i].status=ZOMBIE;
      //sprint("free p:%d",procs[i].pid);
      //sprint("111\n");
    }
  return 0;
}
//
// implements fork syscal in kernel. added @lab3_1
// basic idea here is to first allocate an empty process (child), then duplicate the
// context and data segments of parent process to the child, and lastly, map other
// segments (code, system) of the parent to child. the stack segment remains unchanged
// for the child.
//
int do_fork( process* parent)
{
  uint64 tp=read_tp();
  sprint( "will fork a child from parent %d.\n", parent->pid );
  process* child = alloc_process();
  //sprint("sp1%x\n",child->trapframe->regs.sp);
  for( int i=0; i<parent->total_mapped_region; i++ ){
    // browse parent's vm space, and copy its trapframe and data segments,
    // map its code segment.
    switch( parent->mapped_info[i].seg_type ){
      case CONTEXT_SEGMENT:
        *child->trapframe = *parent->trapframe;
        break;
      case STACK_SEGMENT:
        memcpy( (void*)lookup_pa(child->pagetable, child->mapped_info[STACK_SEGMENT].va),
          (void*)lookup_pa(parent->pagetable, parent->mapped_info[i].va), PGSIZE );
        break;
      case HEAP_SEGMENT:
      {
        {
        int free_block_filter[MAX_HEAP_PAGES];
        memset(free_block_filter, 0, MAX_HEAP_PAGES);
        uint64 heap_bottom = parent->user_heap.heap_bottom;
        for (int i = 0; i < parent->user_heap.free_pages_count; i++)
        {
          int index = (parent->user_heap.free_pages_address[i] - heap_bottom) / PGSIZE;
          free_block_filter[index] = 1;
        }

        // copy and map the heap blocks
        for (uint64 heap_block = current[tp]->user_heap.heap_bottom;
             heap_block < current[tp]->user_heap.heap_top; heap_block += PGSIZE)
        {
          if (free_block_filter[(heap_block - heap_bottom) / PGSIZE]) // skip free blocks
            continue;
          
           //sprint("heap:%x\n",heap_block);
          pte_t *pte;
          pte = page_walk((pagetable_t)child->pagetable, heap_block, 1);
          //sprint("%x\n",pte);
          *pte=0x100;
          //sprint("pte:%x\n",*pte);
          //sprint("heap:%x\n",heap_block);
          //void* child_pa =(void *)parent_pa;
          // void *child_pa;
          // memcpy(child_pa, (void*)lookup_pa(parent->pagetable, heap_block), PGSIZE);
          // COW: just map (not cp) heap here
          uint64 child_pa = lookup_pa(parent->pagetable, heap_block);
          user_vm_map((pagetable_t)child->pagetable, heap_block, PGSIZE, child_pa, prot_to_type(PROT_READ | PROT_COW, 1));
        }
       }
      

        child->mapped_info[HEAP_SEGMENT].npages = parent->mapped_info[HEAP_SEGMENT].npages;

        // copy the heap manager from parent to child
        memcpy((void*)&child->user_heap, (void*)&parent->user_heap, sizeof(parent->user_heap));
        break;
      }
      case CODE_SEGMENT:
        // TODO (lab3_1): implment the mapping of child code segment to parent's
        // code segment.
        // hint: the virtual address mapping of code segment is tracked in mapped_info
        // page of parent's process structure. use the information in mapped_info to
        // retrieve the virtual to physical mapping of code segment.
        // after having the mapping information, just map the corresponding virtual
        // address region of child to the physical pages that actually store the code
        // segment of parent process.
        // DO NOT COPY THE PHYSICAL PAGES, JUST MAP THEM.
        // parent->mapped_info[i]
        
        // lookup_pa(parent->pagetable,parent->mapped_info[i].va);
        uint64 va = parent->mapped_info[i].va, size = parent->mapped_info[i].npages * PGSIZE, pa = lookup_pa(parent->pagetable, parent->mapped_info[i].va);
        int perm = prot_to_type(PROT_EXEC | PROT_READ, 1);
        map_pages(child->pagetable, va, size, pa, perm);

        child->mapped_info[child->total_mapped_region].va = parent->mapped_info[i].va;
        child->mapped_info[child->total_mapped_region].npages =parent->mapped_info[i].npages;
        child->mapped_info[child->total_mapped_region].seg_type = CODE_SEGMENT;
        child->total_mapped_region++;
        break;
      case DATA_SEGMENT:
      {
      for (int j = 0; j < parent->mapped_info[i].npages; j++)
      {
        uint64 addr = lookup_pa(parent->pagetable, parent->mapped_info[i].va + j * PGSIZE);
        char *newaddr = alloc_page();
        memcpy(newaddr, (void *)addr, PGSIZE);
        map_pages(child->pagetable, parent->mapped_info[i].va + j * PGSIZE, PGSIZE,
                  (uint64)newaddr, prot_to_type(PROT_WRITE | PROT_READ, 1));
      }

      // after mapping, register the vm region (do not delete codes below!)
      child->mapped_info[child->total_mapped_region].va = parent->mapped_info[i].va;
      child->mapped_info[child->total_mapped_region].npages =
          parent->mapped_info[i].npages;
      child->mapped_info[child->total_mapped_region].seg_type = DATA_SEGMENT;
      child->total_mapped_region++;
      break;
      }
    }
  }

  child->status = READY;
  child->trapframe->regs.a0 = 0;
  child->parent = parent;
  insert_to_ready_queue( child );
  //sprint("sp2%x\n",child->trapframe->regs.sp);
  //sprint("111\n");
  return child->pid;
}
ssize_t process_wait(int pid)
{
  uint64 tp=read_tp();
  int i=0,j=0,flag=0;//
  if(pid==-1)
  {
    // sprint("0000\n");
    for(i=0;i<NPROC;i++)
    {
      //sprint("0000\n");
      if(procs[i].parent!=NULL)
      {
        //procs[i].parent->pid未经过赋值
        
        //sprint("%d\n",procs[i].parent->pid);
        if(procs[i].parent==current[tp])//找到子进程
       { 
           //sprint("101\n");
            flag=1;
          //有前面的插入函数可知 status=FREE时说明这个位置还没有进程因此我们只需判断子进程是否FREE过了
          if(procs[i].status==ZOMBIE)//子进程调用结束是僵尸进程
          {
            procs[i].status==FREE;
            //代表一个子进程已经处理
            return procs[i].pid;
          }
          break;
        } 
      }
      
    }
    //sprint("1111\n");
    if(flag==1)
    {
     
      insert_to_wait_queue(current[tp]);
       
      schedule(0);
      //sprint("101\n");
      return procs[i].pid;
    }
  }
  else if(pid>0)
  {
    for(i=0;i<NPROC;i++)
    {
      if(procs[i].parent!=NULL)
      {

      
      if(procs[i].parent->pid==current[tp]->pid)//找到子进程
      {
        if(procs[i].pid==pid)
        { 
          flag=1;
          //有前面的插入函数可知 status=FREE时说明这个位置还没有进程因此我们只需判断子进程是否FREE过了
          if(procs[i].status==ZOMBIE)//子进程调用结束是僵尸进程
          {
            procs[i].status==FREE;
            return pid;
          }
          break;    
        }   
      }
      }
    }
    if(flag==1)
    {
      insert_to_wait_queue(current[tp]);//两个都在it里因此会出现碰到process0以为当前调度已经完成的情况
      //sprint("procs[i].sp:%x\n",procs[i].trapframe->regs.sp);
      schedule(0);
      return pid;
    }
    //return -1;
  }
  else 
    return -1;
  return -1;
}