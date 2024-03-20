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
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "proc_file.h"
#include "elf.h"
#include "spike_interface/spike_utils.h"


extern symbol sh[64];
extern elf_sect_header ini;
extern uint64 cot;
extern uint64 id[64];
extern uint64 g_ufree_page;
typedef struct block_m{
  int flag;//0为空闲
  uint64 start;
  uint64 end;
  int index;
}block;
typedef struct page_m{
  block b[100];
  block c[100];
  int n;
  //int value[100];
  int vaild;//为0代表没有启用
}page; 
int first=0;
int p_m=0;//页数
int leave;
page p[100];
uint64 g_malloc_page=0;
uint64  block_alloc(page *pi,int a1)
{
  int i=0,j=1;

  for(i=1;i<=99;i++)
  {
    if(pi[i].vaild==1)
    {//首先页面已经被使用
      int n=pi[i].n;
      for(j=1;j<=99;j++)
      {
        if(pi[i].b[j].flag==0)
        {
          if(pi[i].b[j].end-pi[i].b[j].start==a1)
          {
            pi[i].b[j].flag=1;
          }
          else if(pi[i].b[j].end-pi[i].b[j].start>a1)
          {
            pi[i].b[j].flag=1;
            uint64 end=pi[i].b[j].end;
            uint64 index=pi[i].b[j].index;
            pi[i].b[j].end=pi[i].b[j].start+a1;
            if(j==n)//说明是链表的尾部了
            {
              pi[i].b[j].index=n+1;
              pi[i].b[n+1].index=n+1;
              pi[i].b[n+1].start=pi[i].b[j].end;
              pi[i].b[n+1].flag=0;
              pi[i].b[n+1].end=end;
              pi[i].n+=1;
              leave=0;
              return p[i].b[j].start;
            }
            else{//下一个节点已经存在
              if(pi[i].b[index].flag==0)//合并情况
              {
                pi[i].b[index].start=end;
                return pi[i].b[j].start;
              }
              else //不合并情况
              {
                pi[i].b[j].index=n+1;
                pi[i].b[n+1].index=n+1;
                pi[i].b[n+1].start=pi[i].b[j].end;
                pi[i].b[n+1].flag=0;
                pi[i].b[n+1].end=pi[i].b[index].start;
                pi[i].n+=1;
                return pi[i].b[j].start;
              }
              
            }
          }
          else
          {//小于的情况要充分利用
              p[i].b[j].flag=0;
              
              return 5;
          }
        }
       
      }
      
    }
  }
  return 0;
}
uint64  block_alloc_va(page *pi,int a1)
{
  int i=0,j=1;

  for(i=1;i<=99;i++)
  {
    if(pi[i].vaild==1)
    {//首先页面已经被使用
      int n=pi[i].n;
      for(j=1;j<=99;j++)
      {
        if(pi[i].c[j].flag==0)
        {
          if(pi[i].c[j].end-pi[i].c[j].start==a1)
          {
            pi[i].c[j].flag=1;
          }
          else if(pi[i].c[j].end-pi[i].c[j].start>a1)
          {
            pi[i].c[j].flag=1;
            uint64 end=pi[i].c[j].end;
            uint64 index=pi[i].c[j].index;
            pi[i].c[j].end=pi[i].c[j].start+a1;
            if(j==n)//说明是链表的尾部了
            {
              pi[i].c[j].index=n+1;
              pi[i].c[n+1].index=n+1;
              pi[i].c[n+1].start=pi[i].c[j].end;
              pi[i].c[n+1].flag=0;
              pi[i].c[n+1].end=end;
              //pi[i].n+=1;
              leave=0;
              return p[i].c[j].start;
            }
            else{//下一个节点已经存在
              if(pi[i].c[index].flag==0)//合并情况
              {
                pi[i].c[index].start=end;
                 leave=0;
                return pi[i].c[j].start;
              }
              else //不合并情况
              {
                pi[i].c[j].index=n+1;
                pi[i].c[n+1].index=n+1;
                pi[i].c[n+1].start=pi[i].c[j].end;
                pi[i].c[n+1].flag=0;
                pi[i].c[n+1].end=pi[i].c[index].start;
                //pi[i].n+=1;
                 leave=0;
                return pi[i].c[j].start;
              }
              
            }
          }
          else{
            pi[i].c[j].flag=1;
            leave=p[i].b[j].end-p[i].b[j].start;
            return 5;
          }
        } 

      }
      
    }
  }
  return 0;
}
int block_free_va(page *pi,uint64 va)
{
  int i=0,j=1;
  for(i=1;i<=99;i++)
  {
    if(pi[i].vaild==1)
    {
      int front=1;//前一个节的序号
 
        if(pi[i].c[j].start==va)
        {
          int index1=pi[i].c[j].index;//下一个序号
          if(j==1)//代表是头节点和va相等
          {
            if(pi[i].c[index1].flag==0)
            {
              int index2=pi[i].c[index1].index;
              pi[i].c[j].index=pi[i].c[index2].index;
              pi[i].b[j].index=pi[i].b[index2].index;
              pi[i].c[j].flag=0;
              pi[i].b[j].flag=0;
              pi[i].c[j].end=pi[i].c[index1].end;
              pi[i].b[j].end=pi[i].b[index1].end;
              
            }
            else
            {
              pi[i].c[j].flag=0;
              pi[i].b[j].flag=0;
              
            }
          }
          else//不是头节点
          {
            if(pi[i].c[j].index==j)//尾节点因为尾部指向自身
            {
              if(pi[i].c[front].flag==0)
              {
                pi[i].c[front].index=front;
                pi[i].c[front].end=pi[i].b[j].end;
                pi[i].b[front].index=front;
                pi[i].b[front].end=pi[i].b[j].end;
                
              }
              else{
                  pi[i].c[j].flag=0;
                  pi[i].b[j].flag=0;
                  
              }
            }
            else//中间节点j!=1且index不是自己
            {
               int index3=p[i].c[j].index;//下一个节点
               if(pi[i].c[front].flag==0)
               {
                  if(pi[i].c[index3].flag==0)
                  {
                    pi[i].c[front].end=pi[i].c[index3].end;
                    pi[i].c[front].index=pi[i].c[index3].index;
                    pi[i].b[front].end=pi[i].b[index3].end;
                    pi[i].b[front].index=pi[i].b[index3].index;
                   
                  }
                  else{
                    pi[i].c[front].end=pi[i].c[j].end;
                    pi[i].c[front].index=index3;
                    pi[i].b[front].end=pi[i].b[j].end;
                    pi[i].b[front].index=index3;
                    
                  }
               }
               else//前一个节点不为空闲
               {
                   if(pi[i].c[index3].flag==0)
                  {
                    pi[i].c[j].end=pi[i].c[index3].end;
                    pi[i].c[j].index=pi[i].c[index3].index;
                    p[i].c[j].flag=0;
                    pi[i].b[j].end=pi[i].b[index3].end;
                    pi[i].b[j].index=pi[i].b[index3].index;
                    p[i].b[j].flag=0;
                    
                  }
                  else{
                    p[i].c[j].flag=0;
                    p[i].b[j].flag=0;
                    
                  }
               }
            }
          }
        }
        front=j;
        j=p[i].c[j].index;
      }
    
  }
  return 1;
}

uint64 sys_user_better_allocate_page(uint64 a1) {
  //虚拟地址是va 物理地址pa
  //由pgsize可知一个物理页面是4k的大小分配前应该判断当前物理页面是否用完9+
  int i=0,j=0;
 

  //sprint("start_va:%x\n",g_ufree_page);
  if(first==0)
  {
    void* pa = alloc_page();
    uint64 va = g_ufree_page;
    g_malloc_page=g_ufree_page;
    //g_ufree_page += a1;
    //uint64  
    first=1;
   for(i=1;i<=99;i++)
   {
      if(p[i].vaild==0)
      {
        p[i].n=1;
        p[i].vaild=1;
        p[i].c[1].start=g_ufree_page;//起始地址va
        p[i].c[1].end=g_ufree_page+PGSIZE;
        p[i].c[1].index=p[i].n;
        g_ufree_page+=a1;
      
        p[i].b[1].start=(uint64) pa;
        p[i].b[1].end=p[i].b[1].start+PGSIZE; 
        //p[i].n=1;
        p[i].b[1].index=p[i].n;  
        if(block_alloc_va(p,a1)==5)
        {
          block_alloc(p,a1);
          a1=leave;
        }
        else
            break;
      }
   }
   //sprint("va1:%x\n",va);
   for(j=1;j<=i;j++)
  {
    
    user_vm_map((pagetable_t)current->pagetable, g_malloc_page, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));
    g_malloc_page+=PGSIZE;     
    pa=alloc_page();
  }
         //sprint("first:va:%x\n",va);
   return va;
  }
  else 
  {
    while(1)
    {
          // for(i=1;i<=3;i++)
    // {
    //   sprint("blockstart %d:%x\n",i,p[1].b[i].start);
    //   sprint("blockend %d:%x\n",i,p[1].b[i].end);
    // }
    uint64 va=block_alloc_va(p,a1);
    if(va!=0&&va!=5)
    {
      uint64 pa =block_alloc(p,a1);
      //g_ufree_page += a1;
    // user_vm_map((pagetable_t)current->pagetable, va, a1, pa,
    // prot_to_type(PROT_WRITE | PROT_READ, 1));
    //sprint("second va:%x\n",va);
    //sprint("11\n");
         return va;
    }
    else if(va==5)
    {
      a1=leave;
      block_alloc_va(p,a1);
      uint64 pa =block_alloc(p,a1);
      //sprint("2222\n");
    }
    else{
      //sprint("33\n");
    void* pa = alloc_page();
    uint64 va = g_ufree_page;
    //sprint("va2:%x\n",va);
    //sprint("111\n");

    user_vm_map((pagetable_t)current->pagetable, g_malloc_page, PGSIZE, (uint64)pa,
    prot_to_type(PROT_WRITE | PROT_READ, 1));
    g_malloc_page+=PGSIZE; 
    // user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
    // prot_to_type(PROT_WRITE | PROT_READ, 1));
      for(i=1;i<=99;i++)
   {
      if(p[i].vaild==0)
      {
        p[i].n=1;
        p[i].vaild=1;
        p[i].c[1].start=g_ufree_page;//起始地址va
        p[i].c[1].end=g_ufree_page+PGSIZE;
        p[i].c[1].index=p[i].n;
        g_ufree_page+=a1;
        block_alloc_va(p,a1);
        p[i].b[1].start=(uint64) pa;
        p[i].b[1].end=p[i].b[1].start+a1; 
        //p[i].n=1;
        p[i].b[1].index=p[i].n;
        block_alloc(p,a1);
        break;
      }
   }
   //sprint("leave:%d\n",leave);
   //sprint("va:%x\n",va);
   //sprint("llll\n");
   //sprint("no\n");
   if(leave==0)
    {
      //sprint("yes\n");
      return va;
      //sprint("11\n");
    }

  }
  }

  
  }
  
}
//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_better_free_page(uint64 va) {
  // user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  
  block_free_va(p,va);
  //sprint("444\n");
  return 0;
}

ssize_t sys_user_lab1_challenge1(uint64 a1)
{
  //sprint("in syscall\n");
  
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
 //uint64 ar1=current->trapframe->regs.sp+16;//s0寄存器保存函数调用的栈顶
 //sprint("%x\n",current->trapframe->regs.sp);

 //10082对应的是print_trace的ra ra-8里面存的是fp
 uint64 va=current->trapframe->regs.sp;
  uint64 ar1=0;
//sprint("%x\n",(va));
// for(i=0;i<=20;i++)
// {
//   ar1 = *(uint64 *)user_va_to_pa((pagetable_t)(current->pagetable), (void *)current->trapframe->regs.sp+i*8);
//   sprint("%d %x\n",i,(ar1));
// }
va=va+24;
  ar1 = *(uint64 *)user_va_to_pa((pagetable_t)(current->pagetable), (void *)current->trapframe->regs.sp+3*8);


//  for(i=0;i<=11;i++)
//   sprint("%lx\n",sh[i].offset);
//  sprint("%lx\n",*(uint64 *)(ar1-8));
   while(n<=a1)
    {
    
   for(i=0;sh[i].offset!=1;i++)
   {
      //sprint("%x\n",sh[i].offset);
      //sprint("%x\n",sh[i+1].offset);
       //sprint("%x\n",*(uint64 *)(ar1-8));
      if(ar1>=sh[i].offset&&ar1<sh[i+1].offset)
      {
        sprint("%s\n",sh[i].name);
        break;
      }
     }
     va=va+16;
     ar1 = *(uint64 *)user_va_to_pa((pagetable_t)(current->pagetable),(void *) va);
     //ar1= *(uint64 *)(ar1-16);
      n+=1;
  }
//  for(i=0;i<=10;i++)
//   {
//   sprint("%ld号地址内容:%lx\n",i,*(uint64 *)(0x810fff70-i*8));
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


void syscall_load_bincode_from_host_elf(process *p,char *s,char *para) {
  //arg_buf arg_bug_msg;

  // retrieve command line arguements
  //size_t argc = parse_args(&arg_bug_msg);
  // if (!argc) panic("You need to specify the application program!\n");

  // sprint("%c\n",*s);
  char* path = (char*)user_va_to_pa((pagetable_t)(p->pagetable), s);
  sprint("Application: %s\n", path);

  //elf loading. elf_ctx is defined in kernel/elf.h, used to track the loading process.
  elf_ctx elfloader;
  // elf_info is defined above, used to tie the elf file and its corresponding process.
  elf_info_vs info;
  // sprint("11111\n");
  //char * pathpa = (char*)user_va_to_pa((pagetable_t)(p->pagetable),arg_bug_msg.argv[0]);
  //user_vm_unmap((pagetable_t)(p->pagetable),p->user_heap.heap_bottom,PGSIZE,1);
  
  char *pa=user_va_to_pa((pagetable_t)(p->pagetable),para);
  char *pathpa[2];
  uint64 vsp;
  vsp=p->trapframe->regs.sp;
  vsp-=8;
  
  //char *st;
  //sprint("st:%x\n",st);
  char * sp=user_va_to_pa((pagetable_t)(p->pagetable),(void *)vsp);
  memcpy((void *)sp,(void *)pa,strlen(pa));
  //sprint("%x\n",strlen(pa));
  uint64 vsp1=vsp;
  vsp-=8;
  //sprint("%x\n",sp);
  sp=user_va_to_pa((pagetable_t)(p->pagetable),(void *)vsp);
 // sprint("%x\n",sp);
  uint64 *sp2=(uint64 *)sp;
  //sprint("vsp:%x\n",vsp);
  *sp2=(vsp+8);
  //sprint("sp:%x\n",*sp);
  //sprint("sp:%x\n",*(uint64 *)sp);
  //sp=user_va_to_pa((pagetable_t)(p->pagetable),(void *)vsp);
  //*sp=(vsp)
  int len=1;
  p->trapframe->regs.sp-=16;
  p->trapframe->regs.a0=(uint64)len;
  
  //sprint("%x\n",p->trapframe->regs.a0);
  p->trapframe->regs.a1=(uint64)(vsp);
  //sprint("%x\n",vsp);
  info.f = vfs_open(path, O_RDONLY);
  //sprint("open is ok\n");
  //sprint("11111\n");
  info.p = p;
  // IS_ERR_VALUE is a macro defined in spike_interface/spike_htif.h
  if (IS_ERR_VALUE(info.f)) panic("Fail on openning the input application program.\n");

  // init elfloader context. elf_init() is defined above.
  
  if (elf_init(&elfloader, &info) != EL_OK)
    panic("fail to init elfloader.\n");
 
  // load elf. elf_load() is defined above.
  if (elf_load(&elfloader) != EL_OK) panic("Fail on loading elf.\n");
  //sprint("11111\n");
  // entry (virtual, also physical in lab1_x) address
  p->trapframe->epc = elfloader.ehdr.entry;
  
  

  //sprint("a1 is ok\n");
  // close the host spike file
  vfs_close( info.f );

  //sprint("pathpa[1]:%s\n",pathpa[1]);
  
  //sprint("yes,,,\n");
  sprint("Application program entry point (virtual address): 0x%lx\n", p->trapframe->epc);
  //switch_to(current);
}
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}



//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // reclaim the current process, and reschedule. added @lab3_1
  free_process( current );
  // if(current->pid==0)
  //   sprint("yes_free\n");
  schedule(1);
  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  uint64 va;
  // if there are previously reclaimed pages, use them first (this does not change the
  // size of the heap)
  if (current->user_heap.free_pages_count > 0) {
    va =  current->user_heap.free_pages_address[--current->user_heap.free_pages_count];
    assert(va < current->user_heap.heap_top);
  } else {
    // otherwise, allocate a new page (this increases the size of the heap by one page)
    va = current->user_heap.heap_top;
    current->user_heap.heap_top += PGSIZE;

    current->mapped_info[HEAP_SEGMENT].npages++;
  }
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  // add the reclaimed page to the free page list
  current->user_heap.free_pages_address[current->user_heap.free_pages_count++] = va;
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current );
}
ssize_t sys_user_wait(int pid)
{
  //sprint("111\n");
  return process_wait(pid);
}

//
// kerenl entry point of yield. added @lab3_2
//
ssize_t sys_user_yield() {
  // TODO (lab3_2): implment the syscall of yield.
  // hint: the functionality of yield is to give up the processor. therefore,
  // we should set the status of currently running process to READY, insert it in
  // the rear of ready queue, and finally, schedule a READY process to run.
  insert_to_ready_queue(current);
  schedule(0);
  // panic( "You need to implement the yield syscall in lab3_2.\n" );

  return 0;
}

//
// open file
//
ssize_t sys_user_open(char *pathva, int flags) {
  char* pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  char pathb[30];
  if(*pathpa=='.'&&*(pathpa+1)=='.')
  {
    strcpy(pathb,current->pfiles->cwd->name);
  }
  else if(*pathpa=='.'&&*(pathpa+1)!='.')
  {
    strcpy(pathb,current->pfiles->cwd->name);
    if(strcmp(pathb,"/")==0)//上级目录是根目录相对路径等于当前路径
    {
      strcpy(pathb,pathpa+1);
    }
    else 
    {
     strcat(pathb,pathpa+1);
    }
    //sprint("111\n");
  }
  //sprint("111\n");
  return do_open(pathb, flags);
}

//
// read file
//
ssize_t sys_user_read(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = lookup_pa((pagetable_t)current->pagetable, addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_read(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// write file
//
ssize_t sys_user_write(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = lookup_pa((pagetable_t)current->pagetable, addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_write(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// lseek file
//
ssize_t sys_user_lseek(int fd, int offset, int whence) {
  return do_lseek(fd, offset, whence);
}

//
// read vinode
//
ssize_t sys_user_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current->pagetable), istat);
  return do_stat(fd, pistat);
}

//
// read disk inode
//
ssize_t sys_user_disk_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current->pagetable), istat);
  return do_disk_stat(fd, pistat);
}

//
// close file
//
ssize_t sys_user_close(int fd) {
  return do_close(fd);
}

//
// lib call to opendir
//
ssize_t sys_user_opendir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  return do_opendir(pathpa);
}

//
// lib call to readdir
//
ssize_t sys_user_readdir(int fd, struct dir *vdir){
  struct dir * pdir = (struct dir *)user_va_to_pa((pagetable_t)(current->pagetable), vdir);
  return do_readdir(fd, pdir);
}

//
// lib call to mkdir
//
ssize_t sys_user_mkdir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  return do_mkdir(pathpa);
}

//
// lib call to closedir
//
ssize_t sys_user_closedir(int fd){
  return do_closedir(fd);
}

//
// lib call to link
//
ssize_t sys_user_link(char * vfn1, char * vfn2){
  char * pfn1 = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn1);
  char * pfn2 = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn2);
  return do_link(pfn1, pfn2);
}

//
// lib call to unlink
//
ssize_t sys_user_unlink(char * vfn){
  char * pfn = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn);
  return do_unlink(pfn);
}

ssize_t sys_user_exec(char *s,char *para)
{
  //sprint("o\n");
  char *path=s;

  //sprint();
  //sprint("%s\n",apathpa);
  /// /bin/app_mkdir
  // int fd=do_open(pathpa,O_RDWR | O_CREAT);
  //process *st=alloc_page();
  // current
  syscall_load_bincode_from_host_elf(current,s,para);
  // //sprint("fd:%d\n",fd);
  // //sprint("0\n");
  return -1;
 
}
ssize_t sys_user_ccwd(char *path){
  //char *path1=path;
  char *path1=user_va_to_pa(current->pagetable,path);//查询到当前的进程路径
  if(*path1=='.'&&*(path1+1)=='.')
    strcpy(current->pfiles->cwd->name,"/");
  else if(*path1=='.'&&*(path1+1)=='/')
  {
   //sprint("change1:%s\n",path1);
    strcpy(current->pfiles->cwd->name,path1+1);
    //sprint("change1:%s\n",path);
    //strcpy(user)
  }
  return 0;
}
//路径读
ssize_t sys_user_rcwd(char *path){
  
 
  //sprint("read1:%s\n",user_va_to_pa(current->pagetable,path))
  //存在path时要放在实地址
  strcpy((char*)user_va_to_pa(current->pagetable,path),current->pfiles->cwd->name);
  //strcpy((char*)path,current->pfiles->cwd->name);
  return 0;
}
//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  //("111\n");
  switch (a0) {
   
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    case SYS_user_fork:
      return sys_user_fork();
    case SYS_user_yield:
      return sys_user_yield();
    // added @lab4_1
    case SYS_user_open:
      return sys_user_open((char *)a1, a2);
    case SYS_user_read:
      return sys_user_read(a1, (char *)a2, a3);
    case SYS_user_write:
      return sys_user_write(a1, (char *)a2, a3);
    case SYS_user_lseek:
      return sys_user_lseek(a1, a2, a3);
    case SYS_user_stat:
      return sys_user_stat(a1, (struct istat *)a2);
    case SYS_user_disk_stat:
      return sys_user_disk_stat(a1, (struct istat *)a2);
    case SYS_user_close:
      return sys_user_close(a1);
    // added @lab4_2
    case SYS_user_opendir:
      return sys_user_opendir((char *)a1);
    case SYS_user_readdir:
      return sys_user_readdir(a1, (struct dir *)a2);
    case SYS_user_mkdir:
      return sys_user_mkdir((char *)a1);
    case SYS_user_closedir:
      return sys_user_closedir(a1);
    // added @lab4_3
    case SYS_user_link:
      return sys_user_link((char *)a1, (char *)a2);
    case SYS_user_unlink:
      return sys_user_unlink((char *)a1);
    case  SYS_user_wait:
      return sys_user_wait(a1);
    case SYS_user_exec:
      return sys_user_exec((char *)a1,(char *)a2);
    case SYS_user_lab1_challenge1:
      return sys_user_lab1_challenge1(a1);
    case SYS_user_ccwd:
      return sys_user_ccwd((char *)a1);
    case SYS_user_rcwd:
      return sys_user_rcwd((char *)a1);
    case SYS_user_allocate:
      return  sys_user_better_allocate_page(a1);//a1代表分配空间的大小
    case SYS_user_free:
      return sys_user_better_free_page(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
