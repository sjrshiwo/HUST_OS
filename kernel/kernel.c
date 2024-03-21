/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"
#include "util/types.h"
#include "vfs.h"
#include "rfs.h"
#include "ramdev.h"
#include "sync_utils.h"
volatile int barrier=0;
extern int vm_alloc_stage[NCPU];
//
// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point of
// S-mode trap vector). added @lab2_1
//
extern char trap_sec_start[];

//
// turn on paging. added @lab2_1
//
void enable_paging() {
  // write the pointer to kernel page (table) directory into the CSR of "satp".
  write_csr(satp, MAKE_SATP(g_kernel_pagetable));

  // refresh tlb to invalidate its content.
  flush_tlb();
}

typedef union {
  uint64 buf[MAX_CMDLINE_ARGS];
  char *argv[MAX_CMDLINE_ARGS];
} arg_buf;

//
// returns the number (should be 1) of string(s) after PKE kernel in command line.
// and store the string(s) in arg_bug_msg.
//
static size_t parse_args(arg_buf *arg_bug_msg) {
  // HTIFSYS_getmainvars frontend call reads command arguments to (input) *arg_bug_msg
  long r = frontend_syscall(HTIFSYS_getmainvars, (uint64)arg_bug_msg,
      sizeof(*arg_bug_msg), 0, 0, 0, 0, 0);
  kassert(r == 0);

  size_t pk_argc = arg_bug_msg->buf[0];
  uint64 *pk_argv = &arg_bug_msg->buf[1];

  int arg = 1;  // skip the PKE OS kernel string, leave behind only the application name
  for (size_t i = 0; arg + i < pk_argc; i++)
    arg_bug_msg->argv[i] = (char *)(uintptr_t)pk_argv[arg + i];

  //returns the number of strings after PKE kernel in command line
  return pk_argc - arg;
}

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
process* load_user_program() {
  process* proc;
  //USER_STACK_TOP 
  uint64 tp=read_tp();
  
  proc = alloc_process();
  proc->trapframe->regs.tp=tp;
  sprint("hartid = %d:User application is loading.\n", tp);

  arg_buf arg_bug_msg;
 
 
  // retrieve command line arguements
  size_t argc = parse_args(&arg_bug_msg);
  if (!argc) panic("You need to specify the application program!\n");
//sprint("\npagetable:%x\n",proc->pagetable);
 //sprint("\ns0:%x\n",proc->trapframe->regs.s0);
  load_bincode_from_host_elf(proc, arg_bug_msg.argv[tp]);
  //sprint("\npagetable:%x\n",proc->pagetable);
  //sprint("id%d tp%d\n", proc->pid,tp);
 
  return proc;
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
int s_start(void) {
  sprint("Enter supervisor mode...\n");
  // in the beginning, we use Bare mode (direct) memory mapping as in lab1.
  // but now, we are going to switch to the paging mode @lab2_1.
  // note, the code still works in Bare mode when calling pmm_init() and kern_vm_init().
  write_csr(satp, 0);
  uint64 tp=read_tp();
  if (tp == 0)
  {
    // init phisical memory manager
    pmm_init();

    // build the kernel page table
    kern_vm_init();

    enable_paging();

    // added @lab3_1
    init_proc_pool();

    // init file system, added @lab4_1
    fs_init();
  }
  sync_barrier(&barrier, NCPU);
  // init phisical memory manager
  sprint("Hartid = %d:Switch to user mode...\n",tp);
  // the application code (elf) is first loaded into memory, and then put into execution
  // added @lab3_1
 
  insert_to_ready_queue( load_user_program() ); 
  vm_alloc_stage[tp] = 1;
  schedule(0);
  //schedule(1);
  // we should never reach here.
  return 0;
}
