/*
 * routines that scan and load a (host) Executable and Linkable Format (ELF) file
 * into the (emulated) memory.
 */

#include "elf.h"
#include "string.h"
#include "riscv.h"
#include "spike_interface/spike_utils.h"

symbol sh[32];
elf_section ini;
uint64 cot;
uint64 id[64];
typedef struct elf_info_t {
  spike_file_t *f;
  process *p;
} elf_info;

//
// the implementation of allocater. allocates memory space for later segment loading
//
static void *elf_alloc_mb(elf_ctx *ctx, uint64 elf_pa, uint64 elf_va, uint64 size) {
  // directly returns the virtual address as we are in the Bare mode in lab1_x
  return (void *)elf_va;
}

//
// actual file reading, using the spike file interface.
//
static uint64 elf_fpread(elf_ctx *ctx, void *dest, uint64 nb, uint64 offset) {
  elf_info *msg = (elf_info *)ctx->info;
  // call spike file utility to load the content of elf file into memory.
  // spike_file_pread will read the elf file (msg->f) from offset to memory (indicated by
  // *dest) for nb bytes.
  return spike_file_pread(msg->f, dest, nb, offset);
}

//unsigned cha
// init elf_ctx, a data structure that loads the elf.
//
elf_status elf_init(elf_ctx *ctx, void *info) {
  ctx->info = info;

  // load the elf header
  if (elf_fpread(ctx, &ctx->ehdr, sizeof(ctx->ehdr), 0) != sizeof(ctx->ehdr)) return EL_EIO;

  // check the signature (magic value) of the elf
  if (ctx->ehdr.magic != ELF_MAGIC) return EL_NOTELF;
 
  return EL_OK;
}


//there3

elf_status elf_copyhead(elf_ctx *ctx) {


//得到shrstrndx的地址从而得到查询所有节头表的基地址
uint64 shr_offset=ctx->ehdr.shoff+ctx->ehdr.shstrndx*sizeof(elf_section); //uint64
uint64 sect_count=ctx->ehdr.shnum;

  uint64 i;
  elf_section tp,sym,str,shr;
  elf_fpread(ctx,(void *)&shr,sizeof(shr),shr_offset);
  char shr_sy[shr.sh_size];
  elf_fpread(ctx,&shr_sy,shr.sh_size,shr.sh_offset);
  //cot=sect_count;
  for(i=0;i<sect_count;i++)
  {
    elf_fpread(ctx,(void *)&tp,sizeof(tp),ctx->ehdr.shoff+i*ctx->ehdr.shentsize);
    
    if(strcmp(shr_sy+tp.sh_name,".symtab")==0)
    {
        sym=tp;//sym节头表中存符号和对应地址
        
    }
    else if(strcmp(shr_sy+tp.sh_name,".strtab")==0)
    {
        str=tp;
    }
  }
  //ini=sym;
  //在strtab中找sym地址对应的源程序符
  // elf_sym
  elf_sym sym_t;//每个符号项
  //存所有函数项offset+name
  int j=0;
  for(i=0;i<sym.sh_size/(sizeof(elf_sym));i++)
  {
    //将当前符号项i暂存到sym_t中
    elf_fpread(ctx,(void *)&sym_t,sizeof(elf_sym),sym.sh_offset+i*sizeof(elf_sym));
      id[i]=sym_t.sy_info;
    if(sym_t.sy_info==FUNC)//确认是symatb中的func部分
    {
      // ini=sym;
      // cot=0x0000000000000002;
      //写symbol数组value部分为地址
      sh[j].offset=sym_t.sy_value;
      //在strtab中查找名字
      char name_x[32];
      elf_fpread(ctx,(void *)name_x,sizeof(name_x),str.sh_offset+sym_t.sy_name);
      strcpy(sh[j++].name,name_x);
         
    }
    sh[j].offset=0x1;
  }
  
  return EL_OK;
}
void sort(symbol sh[])
{
    symbol s;  
    int i,j,len=0;
    for(i=0;i<=31;i++)
    {
      if(sh[i].offset==0x1)
      {
        len=i;
        break;
      }
       
    }
    
    //冒泡排序大号在后
    for(i=0;i<len;i++)
    {
      for(j=0;j<len-i;j++)
      {
        if(sh[j].offset>sh[j+1].offset)
        {
          s.offset=sh[j].offset;
          strcpy(s.name,sh[j].name);
          sh[j].offset=sh[j+1].offset;
          strcpy(sh[j].name,sh[j+1].name);
          sh[j+1].offset=s.offset;
          strcpy(sh[j+1].name,s.name);
        }
      }
    }   
}

//
// load the elf segments to memory regions as we are in Bare mode in lab1
//

elf_status elf_load(elf_ctx *ctx) {
  // elf_prog_header structure is defined in kernel/elf.h
  elf_prog_header ph_addr;
  int i, off;

  // traverse the elf program segment headers
  for (i = 0, off = ctx->ehdr.phoff; i < ctx->ehdr.phnum; i++, off += sizeof(ph_addr)) {
    // read segment headers
    if (elf_fpread(ctx, (void *)&ph_addr, sizeof(ph_addr), off) != sizeof(ph_addr)) return EL_EIO;

    if (ph_addr.type != ELF_PROG_LOAD) continue;
    if (ph_addr.memsz < ph_addr.filesz) return EL_ERR;
    if (ph_addr.vaddr + ph_addr.memsz < ph_addr.vaddr) return EL_ERR;

    // allocate memory block before elf loading
    void *dest = elf_alloc_mb(ctx, ph_addr.vaddr, ph_addr.vaddr, ph_addr.memsz);

    // actual loading
    if (elf_fpread(ctx, dest, ph_addr.memsz, ph_addr.off) != ph_addr.memsz)
      return EL_EIO;
  }

  return EL_OK;
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
// load the elf of user application, by using the spike file interface.
//
void load_bincode_from_host_elf(process *p) {
  arg_buf arg_bug_msg;

  // retrieve command line arguements
  size_t argc = parse_args(&arg_bug_msg);
  if (!argc) panic("You need to specify the application program!\n");

  sprint("Application: %s\n", arg_bug_msg.argv[0]);

  //elf loading. elf_ctx is defined in kernel/elf.h, used to track the loading process.
  elf_ctx elfloader;
  // elf_info is defined above, used to tie the elf file and its corresponding process.
  elf_info info;

  info.f = spike_file_open(arg_bug_msg.argv[0], O_RDONLY, 0);
  info.p = p;
  // IS_ERR_VALUE is a macro defined in spike_interface/spike_htif.h
  if (IS_ERR_VALUE(info.f)) panic("Fail on openning the input application program.\n");

  // init elfloader context. elf_init() is defined above.
  if (elf_init(&elfloader, &info) != EL_OK)
    panic("fail to init elfloader.\n");

  // load elf. elf_load() is defined above.
  if (elf_load(&elfloader) != EL_OK) panic("Fail on loading elf.\n");

  //
  elf_copyhead(&elfloader);
  sort(sh);
  // entry (virtual, also physical in lab1_x) address
  p->trapframe->epc = elfloader.ehdr.entry;

  // close the host spike file
  spike_file_close( info.f );

  sprint("Application program entry point (virtual address): 0x%lx\n", p->trapframe->epc);
}
