/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"
#include "sync_utils.h"
#include "spike_interface/spike_utils.h"
extern int id;
extern int count;
uint64 addr=0; 
int k=0;
int wait_run=0;
// int count=1,all=2;
// process is a structure defined in kernel/process.h
process user_app[2];
int print_1=0;
//process user_app1;
// int id=0;
//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
void load_user_program(process *proc) {
  // USER_TRAP_FRAME is a physical address defined in kernel/config.h
  if(addr==0)
  {
    addr=USER_TRAP_FRAME;
  }

  proc->trapframe = (trapframe *)addr;
  addr+=0x100000;
  memset(proc->trapframe, 0, sizeof(trapframe));
  // USER_KSTACK is also a physical address defined in kernel/config.h
  proc->kstack = addr;
  addr+=0x100000;
  proc->trapframe->regs.sp =addr;
  addr+=0x100000;
  // load_bincode_from_host_elf() is defined in kernel/elf.c
  load_bincode_from_host_elf(proc);
  //addr+=0x100000;
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
int s_start(void) {
  uint64 tp=read_tp();
  sprint("hartid = %d: Enter supervisor mode...\n",tp);
  //sprint("hartid = %x: Enter supervisor mode...\n",current->trapframe->regs.tp);
  // Note: we use direct (i.e., Bare mode) for memory mapping in lab1.
  // which means: Virtual Address = Physical Address
  // therefore, we need to set satp to be 0 for now. we will enable paging in lab2_x.
  // 
  // write_csr is a macro defined in kernel/riscv.h
  write_csr(satp, 0);

  // the application code (elf) is first loaded into memory, and then put into execution
  load_user_program(&user_app[tp]);
  //sprint("process.tp%x\n",user_app.trapframe->regs.tp);
  //sprint("hartid = %x: Switch to user mode...\n",current->trapframe->regs.tp);
  user_app[tp].trapframe->regs.tp=(uint64)tp;
  sprint("hartid = %d: Switch to user mode...\n",tp);
  // switch_to() is defined in kernel/process.c 
  count+=1;//以下id改变
  //sprint("id:%d\n",id);
  //for()
  //sprint("!!!\n");
  
  //
  //sprint("%d\n",user_app[k].trapframe->regs.tp);

  //uint64 tp=read_tp();
 //sprint("tp:%d\n",tp);
  switch_to(&user_app[k++]);
  //sprint("111\n");
  //sprint("1111\n");
  // volatile int *c=(volatile int *)&count;
  // //sync_barrier(c,all);
  // we should never reach here.
  return 0;
}
