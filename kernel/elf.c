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
char debug[8000];
symbol sh[64];
 elf_sect_header ini;
uint64 cot;
uint64 id[64];
typedef struct elf_info_t {
  struct file *f;
  process *p;
} elf_info;
uint64 debug_length; 
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

void read_uleb128(uint64 *out, char **off) {
    uint64 value = 0; int shift = 0; uint8 b;
    for (;;) {
        b = *(uint8 *)(*off); (*off)++;
        value |= ((uint64)b & 0x7F) << shift;
        shift += 7;
        if ((b & 0x80) == 0) break;
    }
    if (out) *out = value;
}
void read_sleb128(int64 *out, char **off) {
    int64 value = 0; int shift = 0; uint8 b;
    for (;;) {
        b = *(uint8 *)(*off); (*off)++;
        value |= ((uint64_t)b & 0x7F) << shift;
        shift += 7;
        if ((b & 0x80) == 0) break;
    }
    if (shift < 64 && (b & 0x40)) value |= -(1 << shift);
    if (out) *out = value;
}
// Since reading below types through pointer cast requires aligned address,
// so we can only read them byte by byte
void read_uint64(uint64 *out, char **off) {
    *out = 0;
    for (int i = 0; i < 8; i++) {
        *out |= (uint64)(**off) << (i << 3); (*off)++;
    }
}
void read_uint32(uint32 *out, char **off) {
    *out = 0;
    for (int i = 0; i < 4; i++) {
        *out |= (uint32)(**off) << (i << 3); (*off)++;
    }
}
void read_uint16(uint16 *out, char **off) {
    *out = 0;
    for (int i = 0; i < 2; i++) {
        *out |= (uint16)(**off) << (i << 3); (*off)++;
    }
}
void make_addr_line(elf_ctx *ctx, char *debug_line, uint64 length) {
   process *p = ((elf_info *)ctx->info)->p;
   
    p->debugline = debug_line;
    // directory name char pointer array
    p->dir = (char **)((((uint64)debug_line + length + 7) >> 3) << 3); int dir_ind = 0, dir_base;
    // file name char pointer array
    p->file = (code_file *)(p->dir + 64); int file_ind = 0, file_base;
    // table array
    p->line = (addr_line *)(p->file + 64); p->line_ind = 0;
    char *off = debug_line;
    while (off < debug_line + length) { // iterate each compilation unit(CU)
        debug_header *dh = (debug_header *)off; off += sizeof(debug_header);
        dir_base = dir_ind; file_base = file_ind;
        // get directory name char pointer in this CU
        while (*off != 0) {
            p->dir[dir_ind++] = off; while (*off != 0) off++; off++;
        }
        off++;
        // get file name char pointer in this CU
        while (*off != 0) {
            p->file[file_ind].file = off; while (*off != 0) off++; off++;
            uint64 dir; read_uleb128(&dir, &off);
            p->file[file_ind++].dir = dir - 1 + dir_base;
            read_uleb128(NULL, &off); read_uleb128(NULL, &off);
        }
        off++; addr_line regs; regs.addr = 0; regs.file = 1; regs.line = 1;
        // simulate the state machine op code
       
        for (;;) {
            uint8 op = *(off++);
            switch (op) {
                case 0: // Extended Opcodes
                //sprint("0\n");
                    read_uleb128(NULL, &off); op = *(off++);
                    switch (op) {
                        case 1: // DW_LNE_end_sequence
                        //sprint("11\n");
                            if (p->line_ind > 0 && p->line[p->line_ind - 1].addr == regs.addr) p->line_ind--;
                            //sprint("22\n");
                            p->line[p->line_ind] = regs; p->line[p->line_ind].file += file_base - 1;
                            p->line_ind++; goto endop;
                        case 2: // DW_LNE_set_address
                        //sprint("22\n");
                            read_uint64(&regs.addr, &off); break;
                        // ignore DW_LNE_define_file
                        case 4: // DW_LNE_set_discriminator
                            //sprint("44\n");
                            read_uleb128(NULL, &off); break;
                    }
                    break;
                case 1: // DW_LNS_copy
                //sprint("11\n");
                    if (p->line_ind > 0 && p->line[p->line_ind - 1].addr == regs.addr) p->line_ind--;
                    //问题出现在下面这句话 Misaligned Load!
                    sprint("%d\n",p->line_ind);
                    p->line[p->line_ind] = regs;  
                    //sprint("%d\n",  p->line[p->line_ind].file );
                    p->line[p->line_ind].file += file_base - 1;
                    //sprint("\n");
                    p->line_ind++; break;
                
                case 2: { // DW_LNS_advance_pc
                //sprint("2\n");
                            uint64 delta; read_uleb128(&delta, &off);
                            regs.addr += delta * dh->min_instruction_length;
                            break;
                        }
                case 3: { // DW_LNS_advance_line
                 //sprint("3\n");
                            int64 delta; read_sleb128(&delta, &off);
                            regs.line += delta; break; } case 4: // DW_LNS_set_file
                        read_uleb128(&regs.file, &off); break;
                case 5: // DW_LNS_set_column
                 //sprint("5\n");
                        read_uleb128(NULL, &off); break;
                case 6: // DW_LNS_negate_stmt
                case 7: // DW_LNS_set_basic_block
                        break;
                case 8: { // DW_LNS_const_add_pc
                 //sprint("8\n");
                
                            int adjust = 255 - dh->opcode_base;
                            int delta = (adjust / dh->line_range) * dh->min_instruction_length;
                            regs.addr += delta; break;
                        }
                case 9: { // DW_LNS_fixed_advanced_pc
                //sprint("9\n");
                            uint16 delta; read_uint16(&delta, &off);
                            regs.addr += delta;
                            break;
                        }
                        // ignore 10, 11 and 12
                default: { // Special Opcodes
                //sprint("default\n");
                             int adjust = op - dh->opcode_base;
                             int addr_delta = (adjust / dh->line_range) * dh->min_instruction_length;
                             int line_delta = dh->line_base + (adjust % dh->line_range);
                             regs.addr += addr_delta;
                             regs.line += line_delta;
                             if (p->line_ind > 0 && p->line[p->line_ind - 1].addr == regs.addr) p->line_ind--;
                             p->line[p->line_ind] = regs; p->line[p->line_ind].file += file_base - 1;
                             p->line_ind++; break;
                         }
            }
        } 
      
endop:;
    }
    // for (int i = 0; i < p->line_ind; i++)
    //     sprint("%p %d %d\n", p->line[i].addr, p->line[i].line, p->line[i].file);
}
void elf_section_read(elf_ctx *ctx)
{
  process *p = ((elf_info *)ctx->info)->p;
   
    //tx->ehdr.shoff=0x33f8;
    uint64 shr_offset=ctx->ehdr.shoff+ctx->ehdr.shstrndx*sizeof(elf_sect_header); //uint64
    uint64 sect_count=ctx->ehdr.shnum;
    uint64 i=0;
    //首先读shr节的内容因为存储了所有的节头的地址信息
    elf_sect_header tp,shr;
    elf_fpread(ctx,(void *)&shr,sizeof(shr),shr_offset);//shr此时就是shstrndx节
    char shr_sy[shr.size];
    elf_fpread(ctx,&shr_sy,shr.size,shr.offset);
    for(i=0;i<sect_count;i++)
    {   
        
        elf_fpread(ctx,(void *)&tp,sizeof(tp),ctx->ehdr.shoff+i*ctx->ehdr.shentsize);
        //每个节的节头指针中name存的内容是索引
        if(strcmp(shr_sy+tp.name,".debug_line")==0)
        {
          //sprint("shoff:%x l:%x i:%d\n",ctx->ehdr.shoff,ctx->ehdr.shentsize,i);
          //sprint("debugname:%d\n",tp.name);
          
          sprint("%x\n",tp.offset);
          uint64 *pa=user_va_to_pa(p->pagetable,(void *)tp.offset);
          sprint("%x\n",pa);
          elf_fpread(ctx,(void *)debug,tp.size,(uint64)pa);
          debug_length=tp.size;
        }
    }
    //process *p = ((elf_info *)ctx->info)->p;
    //sprint("line:%x\n",p->line_ind);
    //sprint("debuglenth:%d\n",debug_length);
    sprint("debug:%x\n",debug);
    //sprint("111\n");
    //sprint("%x\n",*(uint64 *)debug);
    make_addr_line(ctx,debug,debug_length);
    //sprint("111\n");
    // process *p = ((elf_info *)ctx->info)->p;
    // for(i=0;i<=2;i++)
    //  sprint("addr:%x line:%x file:%x\n",p->line[i].addr,p->line[i].line,p->line[i].file);
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
  // for(i=0;i<=len-1;i++)
  // {
  //   sprint("sh.name:%s sh.offset:%x\n",sh[i].name,sh[i].offset);
  // }   
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
  if(strcmp(filename,"/bin/app_backtrace")==0)
  {
    elf_copyhead(&elfloader);
    sort(sh);
  }
  
  //sprint("\np->pagetable:%x\n",p->pagetable);
  if(strcmp(filename,"/bin/app_errorline_challengex")==0)
    elf_section_read(&elfloader);

  // entry (virtual, also physical in lab1_x) address
  p->trapframe->epc = elfloader.ehdr.entry;

  // close the vfs file
  vfs_close( info.f );

  sprint("Application program entry point (virtual address): 0x%lx\n", p->trapframe->epc);
}
