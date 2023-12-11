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
#include "elf.h"
#include "spike_interface/spike_utils.h"
extern symbol sh[32];
extern elf_section ini;
extern uint64 cot;
extern uint64 id[64];
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}
//kstack:0x8120000
//userstack:0x801100000
//USER_KSTACK 0x81200000
ssize_t sys_user_lab1(uint64 a1)
{
  uint64 n=1;
  uint64 i=0; 
  assert(current);
  //uint64 addr1=current->trapframe->kernel_sp;
  //USER_STACK
  //sp是高地址
  //sprint("kstack:%lx\n",current->trapframe->regs.s0);
  //
  //print的栈最高地址0x810fff70 0x810ffff70-8返回地址为0x8100000e即调用print的下一个地址 0x810ffff70-16为0x810fff80下一个函数栈为f7引用f8存f8的返回地址
  //栈标识的fp分别为0x810fff70 0x810fff80 0x810fff90 0x810fffa0...
  //
 uint64 ar1=current->trapframe->regs.s0+16;//s0寄存器保存函数调用的栈顶
//  for(i=0;i<=11;i++)
//   sprint("%lx\n",sh[i].offset);
//  sprint("%lx\n",*(uint64 *)(ar1-8));
   while(n<=a1)
    {
    
   for(i=1;i<=sizeof(sh);i++)
   {
    
      if(*(uint64 *)(ar1-8)>=sh[i].offset&&*(uint64 *)(ar1-8)<sh[i+1].offset)
      {
        //sprint("%ld",i);
        if(strcmp(sh[i].name,"main")==0)
        {
          n=a1;
        }
        //sprint("%lx\n",*(uint64 *)ar1-8);
        sprint("%s\n",sh[i].name);
        break;
      }
     }
      ar1= *(uint64 *)(ar1-16);
      n+=1;
  }
//  for(i=0;i<=3;i++)
//  {
//   sprint("%ld号地址内容:%lx\n",i,*(uint64*)(0x810fff70-i*8));
//      //读栈
//  }
// 读elf
  // for(i=0;i<=10;i++)
  // {
  //   sprint("elf:%s\n",sh[i].name);
  // }
  // sprint("elf_sect:%lx\n",cot);
  //sprint("kstack:%lx\n",current->trapframe->epc);
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)

long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_lab1:
      return sys_user_lab1(a1);
    case SYS_user_exit:
      return sys_user_exit(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
