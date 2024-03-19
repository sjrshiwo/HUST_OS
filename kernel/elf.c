/*
 * routines that scan and load a (host) Executable and Linkable Format (ELF) file
 * into the (emulated) memory.
 */

#include "elf.h"
#include "string.h"
#include "riscv.h"
#include "vmm.h"
#include "pmm.h"
#include "vfs.h"
#include "spike_interface/spike_utils.h"

symbol sh[64];
 elf_sect_header ini;
uint64 cot;
uint64 id[64];
typedef struct elf_info_t {
  struct file *f;
  process *p;
} elf_info;

//
// the implementation of allocater. allocates memory space for later segment loading.
// this allocater is heavily modified @lab2_1, where we do NOT work in bare mode.
//
static void *elf_alloc_mb(elf_ctx *ctx, uint64 elf_pa, uint64 elf_va, uint64 size) {
  elf_info *msg = (elf_info *)ctx->info;
  // we assume that size of proram segment is smaller than a page.
  kassert(size < PGSIZE);
  void *pa = alloc_page();
  if (pa == 0) panic("uvmalloc mem alloc falied\n");

  pte_t *pte;
  pte = page_walk((pagetable_t)msg->p->pagetable, elf_va, 1);
  //kassert(size < 2 * PGSIZE);
  if(*pte & PTE_V)
  {
    //sprint("va:%x\n", elf_va);
    user_vm_unmap((pagetable_t)msg->p->pagetable, elf_va, PGSIZE, 1);

  }
  user_vm_map((pagetable_t)msg->p->pagetable, elf_va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ | PROT_EXEC, 1));
  //sprint("222\n");
  
  return pa;
}

//
// actual file reading, using the vfs file interface.
//
static uint64 elf_fpread(elf_ctx *ctx, void *dest, uint64 nb, uint64 offset) {
  elf_info *msg = (elf_info *)ctx->info;
  vfs_lseek(msg->f, offset, SEEK_SET);
  return vfs_read(msg->f, dest, nb);
}

//
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

//
// load the elf segments to memory regions.
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

    // record the vm region in proc->mapped_info. added @lab3_1
    int j;
    for( j=0; j<PGSIZE/sizeof(mapped_region); j++ ) //seek the last mapped region
      if( (process*)(((elf_info*)(ctx->info))->p)->mapped_info[j].va == 0x0 ) break;

    ((process*)(((elf_info*)(ctx->info))->p))->mapped_info[j].va = ph_addr.vaddr;
    ((process*)(((elf_info*)(ctx->info))->p))->mapped_info[j].npages = 1;

    // SEGMENT_READABLE, SEGMENT_EXECUTABLE, SEGMENT_WRITABLE are defined in kernel/elf.h
    if( ph_addr.flags == (SEGMENT_READABLE|SEGMENT_EXECUTABLE) ){
      ((process*)(((elf_info*)(ctx->info))->p))->mapped_info[j].seg_type = CODE_SEGMENT;
      sprint( "CODE_SEGMENT added at mapped info offset:%d\n", j );
    }else if ( ph_addr.flags == (SEGMENT_READABLE|SEGMENT_WRITABLE) ){
      ((process*)(((elf_info*)(ctx->info))->p))->mapped_info[j].seg_type = DATA_SEGMENT;
      sprint( "DATA_SEGMENT added at mapped info offset:%d\n", j);
    }else
      panic( "unknown program segment encountered, segment flag:%d.\n", ph_addr.flags );

    ((process*)(((elf_info*)(ctx->info))->p))->total_mapped_region ++;
  }

  return EL_OK;
}
elf_status elf_copyhead(elf_ctx *ctx) {

 elf_info *msg = (elf_info *)ctx->info;
 //sprint("\ncopyhead_pagetable:%x\n",msg->p->pagetable);
//得到shrstrndx的地址从而得到查询所有节头表的基地址
uint64 shr_offset=ctx->ehdr.shoff+ctx->ehdr.shstrndx*sizeof(elf_sect_header); //uint64
uint64 sect_count=ctx->ehdr.shnum;

  uint64 i;
  elf_sect_header tp,sym,str,shr;
  elf_fpread(ctx,(void *)&shr,sizeof(shr),shr_offset);
  char shr_sy[shr.size];
  elf_fpread(ctx,&shr_sy,shr.size,shr.offset);
  //cot=sect_count;
  for(i=0;i<sect_count;i++)
  {
    elf_fpread(ctx,(void *)&tp,sizeof(tp),ctx->ehdr.shoff+i*ctx->ehdr.shentsize);
    
    if(strcmp(shr_sy+tp.name,".symtab")==0)
    {
        sym=tp;//sym节头表中存符号和对应地址
        
    }
    else if(strcmp(shr_sy+tp.name,".strtab")==0)
    {
        str=tp;
    }
  }
  //ini=sym;
  //在strtab中找sym地址对应的源程序符
  // elf_sym
  elf_sym sym_t;//每个符号项
  //存所有函数项offset+name
  //sprint("\ncopyhead_pagetable:%x\n",msg->p->pagetable);
  int j=0;
  for(i=0;i<sym.size/(sizeof(elf_sym));i++)
  {
    //将当前符号项i暂存到sym_t中
    elf_fpread(ctx,(void *)&sym_t,sizeof(elf_sym),sym.offset+i*sizeof(elf_sym));
      id[i]=sym_t.sy_info;
    if(sym_t.sy_info==FUNC)//确认是symatb中的func部分
    {
      // ini=sym;
      // cot=0x0000000000000002;
      //写symbol数组value部分为地址
      sh[j].offset=sym_t.sy_value;
      //在strtab中查找名字
      char name_x[32];
      elf_fpread(ctx,(void *)name_x,sizeof(name_x),str.offset+sym_t.sy_name);
      strcpy(sh[j++].name,name_x);
         
    }
    
    sh[j].offset=0x1;
    //sprint("offset:%x\n", &sh[j].offset);
    //sprint("\ncopyhead_pagetable:%x\n",&msg->p->pagetable);
  }
  //sprint("j:%d\n",j);
  //sprint("\ncopyhead_pagetable:%x\n",msg->p->pagetable);
  return EL_OK;
}
void sort(symbol sh[])
{
    symbol s;  
    int i,j,len=0;
    for(i=0;i<=63;i++)
    {
      if(sh[i].offset==0x1)
      {
        len=i;
        break;
      }
       
    }
    //sprint("%d\n",len);
    //冒泡排序大号在后
    for(i=0;i<len-1;i++)
    {
      for(j=0;j<len-1-i;j++)
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
  for(i=0;i<=len-1;i++)
  {
    sprint("sh.name:%s sh.offset:%x\n",sh[i].name,sh[i].offset);
  }   
  //sprint("sh.name:%s sh.offset:%x\n",sh[i].name,sh[i].offset);
}

//
// load the elf of user application, by using the spike file interface.
//
void load_bincode_from_host_elf(process *p, char *filename) {
  sprint("Application: %s\n", filename);

  //elf loading. elf_ctx is defined in kernel/elf.h, used to track the loading process.
  elf_ctx elfloader;
  // elf_info is defined above, used to tie the elf file and its corresponding process.
  elf_info info;

  info.f = vfs_open(filename, O_RDONLY);
  info.p = p;
  // IS_ERR_VALUE is a macro defined in spike_interface/spike_htif.h
  if (IS_ERR_VALUE(info.f)) panic("Fail on openning the input application program.\n");

  // init elfloader context. elf_init() is defined above.
  if (elf_init(&elfloader, &info) != EL_OK)
    panic("fail to init elfloader.\n");

  // load elf. elf_load() is defined above.
  if (elf_load(&elfloader) != EL_OK) panic("Fail on loading elf.\n");
  //sprint("\np->pagetable:%x\n",p->pagetable);
  elf_copyhead(&elfloader);
  //sprint("\np->pagetable:%x\n",p->pagetable);
  sort(sh);
  // entry (virtual, also physical in lab1_x) address
  p->trapframe->epc = elfloader.ehdr.entry;

  // close the vfs file
  vfs_close( info.f );

  sprint("Application program entry point (virtual address): 0x%lx\n", p->trapframe->epc);
}
