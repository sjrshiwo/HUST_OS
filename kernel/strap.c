/*
 * Utility functions for trap handling in Supervisor mode.
 */

#include "riscv.h"
#include "process.h"
#include "strap.h"
#include "syscall.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "util/functions.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"
#include "string.h"
//
// handling the syscalls. will call do_syscall() defined in kernel/syscall.c
//
static void handle_syscall(trapframe *tf) {
  // tf->epc points to the address that our computer will jump to after the trap handling.
  // for a syscall, we should return to the NEXT instruction after its handling.
  // in RV64G, each instruction occupies exactly 32 bits (i.e., 4 Bytes)
  tf->epc += 4;

  // TODO (lab1_1): remove the panic call below, and call do_syscall (defined in
  // kernel/syscall.c) to conduct real operations of the kernel side for a syscall.
  // IMPORTANT: return value should be returned to user app, or else, you will encounter
  // problems in later experiments!
  (tf->regs).a0=do_syscall((tf->regs).a0,(tf->regs).a1,(tf->regs).a2,(tf->regs).a3,(tf->regs).a4,(tf->regs).a5,(tf->regs).a6,(tf->regs).a7);
}

//
// global variable that store the recorded "ticks". added @lab1_3
static uint64 g_ticks = 0;
//
// added @lab1_3
//
void handle_mtimer_trap() {
  sprint("Ticks %d\n", g_ticks);
  // TODO (lab1_3): increase g_ticks to record this "tick", and then clear the "SIP"
  // field in sip register.
  // hint: use write_csr to disable the SIP_SSIP bit in sip.
  g_ticks+=1;
  write_csr(sip, 0);
  

}


// the page fault handler. added @lab2_3. parameters:
// sepc: the pc when fault happens;
// stval: the virtual address that causes pagefault when being accessed.

// void handle_user_page_fault(uint64 mcause, uint64 sepc, uint64 stval)
// {
//   // sprint("handle_page_fault: %lx\n", stval);
//   uint64 pa;
//   uint64 va = ROUNDDOWN(stval, PGSIZE);
//   uint64 tp=read_tp();
//   sprint("%x\n",stval<current[tp]->trapframe->regs.sp);
//   pte_t *pte = page_walk(current[tp]->pagetable, va, 0);
//   switch (mcause)
//   {
//   case CAUSE_STORE_PAGE_FAULT:
//     if (pte == NULL)
//     {
//       pa = (uint64)alloc_page(); // allocate a new physical page
//       if ((void *)pa == NULL)
//         panic("Can not allocate a new physical page.\n");
//       map_pages(current[tp]->pagetable, ROUNDDOWN(stval, PGSIZE), PGSIZE, pa, prot_to_type(PROT_READ | PROT_WRITE, 1)); // maps the new page to the virtual address that causes the page fault
//     }
//     else if (*pte & PTE_C)
//     {
//       void *pa = alloc_page();
//       memcpy(pa, (void *)lookup_pa(current[tp]->pagetable, va), PGSIZE);
//       *pte = PTE_V | PA2PTE(pa) | prot_to_type(PROT_READ | PROT_WRITE, 1);
//       *pte &= ~PTE_C;
//     }
//     else if (stval < USER_STACK_TOP && stval > (USER_STACK_TOP - 20 * STACK_SIZE))
//     {
//       map_pages(current[tp]->pagetable, va, PGSIZE, (uint64)alloc_page(), prot_to_type(PROT_READ | PROT_WRITE, 1));
//       break;
//     }
//     else
//       panic("unknown page fault.\n");
//     break;
//   default:
//     panic("unknown page fault.\n");
//     break;
//   }
// }

void handle_user_page_fault(uint64 mcause, uint64 sepc, uint64 stval) {
  uint64 tp=read_tp();

  sprint("handle_page_fault: %lx\n", stval);
  uint64 pa=lookup_pa(current[tp]->pagetable, stval);
  //sprint("strap:%x\n",current[tp]->trapframe->regs.sp);
  //sprint("pa:%x\n",pa);
  if(stval<current[tp]->trapframe->regs.sp&&!pa)
  {
      //sprint("sp:%x\n",current[tp]->trapframe->regs.sp);
     //超出用户栈
    // if(stval<(uint64)0x7ffff000) 
      // sprint("%lx\n",stval);
    panic("this address is not available!");
  }
  switch (mcause) {
    case CAUSE_STORE_PAGE_FAULT:
    {
      // TODO (lab2_3): implement the operations that solve the page fault to
      // dynamically increase application stack.
      // hint: first allocate a new physical page, and then, maps the new page to the
      // virtual address that causes the page fault.
    //panic( "You need to implement the operations that actually handle the page fault in lab2_3.\n" );
    void* pa = alloc_page();
    pte_t *pte;
    pte = page_walk((pagetable_t)current[tp]->pagetable, stval, 1);
          //sprint("%x\n",pte);
    //sprint("pte_strap:%x\n",*pte);
    //if()
    if(*pte&&0x100!=0)
    {
      *pte=*pte&0xffffeffe;
    }
    map_pages(current[tp]->pagetable, (uint64)stval,(uint64) 1, (uint64)pa,
          prot_to_type(PROT_WRITE | PROT_READ, 1));
          //user_vm_map
    //stval+=PGSIZE;
    //panic("1");
      break;
    }
    default:
      sprint("unknown page fault.\n");
      break;
  }
}

//
// implements round-robin scheduling. added @lab3_3
//
void rrsched() {
  // TODO (lab3_3): implements round-robin scheduling.
  // hint: increase the tick_count member of current process by one, if it is bigger than
  // TIME_SLICE_LEN (means it has consumed its time slice), change its status into READY,
  // place it in the rear of ready queue, and finally schedule next process to run.
  // panic( "You need to further implement the timer handling in lab3_3.\n" );
  uint64 tp=read_tp();
  if(current[tp]->tick_count+1>=2)
  {
    current[tp]->tick_count=0;
    insert_to_ready_queue(current[tp]);
    schedule(0);
  }
  else 
  current[tp]->tick_count+=1;  ;
 
}

//
// kernel/smode_trap.S will pass control to smode_trap_handler, when a trap happens
// in S-mode.
//
void smode_trap_handler(void) {
  // make sure we are in User mode before entering the trap handling.
  // we will consider other previous case in lab1_3 (interrupt).
  if ((read_csr(sstatus) & SSTATUS_SPP) != 0) panic("usertrap: not from user mode");
  uint64 tp=read_tp();
  assert(current[tp]);
  // save user process counter.
  current[tp]->trapframe->epc = read_csr(sepc);

  // if the cause of trap is syscall from user application.
  // read_csr() and CAUSE_USER_ECALL are macros defined in kernel/riscv.h
  uint64 cause = read_csr(scause);

  // use switch-case instead of if-else, as there are many cases since lab2_3.
  switch (cause) {
    case CAUSE_USER_ECALL:
      handle_syscall(current[tp]->trapframe);
      break;
    case CAUSE_MTIMER_S_TRAP:
      handle_mtimer_trap();
      // invoke round-robin scheduler. added @lab3_3
      rrsched();
      break;
    case CAUSE_STORE_PAGE_FAULT:
    case CAUSE_LOAD_PAGE_FAULT:
      // the address of missing page is stored in stval
      // call handle_user_page_fault to process page faults
      handle_user_page_fault(cause, read_csr(sepc), read_csr(stval));
      break;
    default:
      sprint("smode_trap_handler(): unexpected scause %p\n", read_csr(scause));
      sprint("            sepc=%p stval=%p\n", read_csr(sepc), read_csr(stval));
      panic( "unexpected exception happened.\n" );
      break;
  }

  // continue (come back to) the execution of current process.
  switch_to(current[tp]);
}
