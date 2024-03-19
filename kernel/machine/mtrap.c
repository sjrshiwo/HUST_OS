#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"

static void handle_instruction_access_fault() { panic("Instruction access fault!"); }

static void handle_load_access_fault() { panic("Load access fault!"); }

static void handle_store_access_fault() { panic("Store/AMO access fault!"); }

static void handle_illegal_instruction() { panic("Illegal instruction!"); }

static void handle_misaligned_load() { panic("Misaligned Load!"); }

static void handle_misaligned_store() { panic("Misaligned AMO!"); }

// added @lab1_3
static void handle_timer() {
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64*)CLINT_MTIMECMP(cpuid) = *(uint64*)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}

//
// handle_mtrap calls a handling function according to the type of a machine mode interrupt (trap).
//
void handle_mtrap() {
  uint64 mcause = read_csr(mcause);
  if(mcause==CAUSE_ILLEGAL_INSTRUCTION)
  {
    
    addr_line *line = current->line;
    uint64 cause_addr=read_csr(mepc);
  int i=0,dir1=0,line1=0,len=0;
  char s1[1000],s2[1000],buf,s3[1000],s[1000]="\0";
  spike_file_t *fn;
  // for(i=0;current->line[i].addr!='\0';i++)
  // {
  //     if(current->line[i].addr==cause_addr) 
  //     sprint("addr:%x line:%x file:%x\n",current->line[i].addr,current->line[i].line,current->line[i].file);
  // }
  for(i=0;line[i].addr!='\0';i++) 
  {
    if(line[i].addr==cause_addr)
    {
      line1=line[i].line;
      break;
    }
  }
  //sprint("long:%d\n",current->line_ind);
  //sprint("%d %d\n",i,line);
  //sprint("%d\n",current->line[i].file);
  dir1=current->file[line[i].file].dir;
  strcpy(s1,current->file[line[i].file].file);//当前的路径
  //sprint("%d\n",dir1);
  //strcpy(s2,*(current->dir)+dir1);错误
  strcpy(s2,current->dir[dir1]);
  //sprint("s2:%s\n",s2);
  sprint("Runtime error at %s/%s:%d\n",s2,s1,line1);
  strcpy(s,s2);
  //sprint("s2:%s\n",s2);
  strcpy(s+strlen(s),"/");
  strcpy(s+strlen(s),s1);
  //sprint("路径：%s\n",s);
  fn=spike_file_open(s,O_RDONLY,0);
  //if (IS_ERR_VALUE(fn)) panic("Fail on openning the input application program.\n");
  while(len<line1-1)
  {
    spike_file_read(fn,(void *)&buf,1);
    //sprint("%c",buf);
    if(buf=='\n')
      len+=1;
  }
  //sprint("%d\n",len);
  //sprint("11\n");
  //sprint("buf:%c\n",buf);
  // spike_file_read(fn,(void *)&buf,1);
  // spike_file_read(fn,(void *)&buf,1);
  // sprint("%c\n",buf);
  for(i=0;;i++)
  {
    spike_file_read(fn,(void *)&buf,1);
    if(buf=='\n')
    {
      break;
    }
    //sprint("%c",buf);
    s3[i]=buf;
  }
  s3[i]=0;
  spike_file_close(fn);
  sprint("%s\n",s3);

  }
  switch (mcause) {
    case CAUSE_MTIMER:
      handle_timer();
      break;
    case CAUSE_FETCH_ACCESS:
      handle_instruction_access_fault();
      break;
    case CAUSE_LOAD_ACCESS:
      handle_load_access_fault();
    case CAUSE_STORE_ACCESS:
      handle_store_access_fault();
      break;
    case CAUSE_ILLEGAL_INSTRUCTION:
      // TODO (lab1_2): call handle_illegal_instruction to implement illegal instruction
      // interception, and finish lab1_2.
     handle_illegal_instruction();

      break;
    case CAUSE_MISALIGNED_LOAD:
      handle_misaligned_load();
      break;
    case CAUSE_MISALIGNED_STORE:
      handle_misaligned_store();
      break;

    default:
      sprint("machine trap(): unexpected mscause %p\n", mcause);
      sprint("            mepc=%p mtval=%p\n", read_csr(mepc), read_csr(mtval));
      panic( "unexpected exception happened in M-mode.\n" );
      break;
  }
}
