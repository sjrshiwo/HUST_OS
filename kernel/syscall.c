/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "sync_utils.h"
#include "spike_interface/spike_utils.h"
extern int id;
extern int wait_run;
int a=0,b=2,c=0,d=2;

volatile int st=0;
process *last=NULL;
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // uint64 epc=read_csr(sepc);
  uint64 tp=read_tp();
  sprint("hartid = %d: %s",tp, buf);
  //sprint("tp:%d\n",current[tp]->trapframe->regs.tp);
  
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  uint64 tp=read_tp();
  sprint("hartid = %d:User exit with code:%d.\n", tp,code);
  sync_barrier(&st,NCPU);
  if(tp==0)
  {
  sprint("hartid = %d: shutdown with code:%d.\n", tp,code);
  shutdown(code);
  
  }
  return 1;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    default:
    {
      //sprint("tp:%d\n",current->trapframe->regs.tp);
       panic("Unknown syscall %ld \n", a0);
    }
     
  }
}
