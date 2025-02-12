/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>
#include "sync_utils.h"
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
#include "spike_interface/spike_file.h"
int sem_s=0;
extern process* ready_queue_head;
volatile int count_4=0;
volatile int count_7=0;
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

int l[100];//存信号量
int len=0;//信号量到多少
uint64 readtp()
{
  uint64 tp=read_tp();
  return tp;
}
//lab3_challenge2
int sys_user_sem_new(int a1)
{
  len++;
  l[len]=a1;
  //sprint("%d %d\n",len,l[len]);
  return len;
}
int sys_user_sem_P(int a1)
{
  //sprint("l[1]:%d\n",l[1]);
  //sprint("l[2]:%d\n",l[2]);
  //sprint("l[3]:%d\n",l[3]);

  if(l[a1]-1>=0)
  {
    l[a1]-=1;
    //sprint("111\n");
  }
  else
  {
    l[a1]-=1;
    //sprint("charu\n");
    current[readtp()]->sem=a1;
    //current->status=BLOCKED;
    insert_to_wait_queue(current[readtp()]);
    schedule(0);
  }
  return 1;
}
//insert_to_ready_queue
int sys_user_sem_V(int a1)
{
  sem_s=1;
  l[a1]+=1;
  wake_wait_queue(a1);//将下一个子进程插入了就绪队列
  //if(l[a1]>0) l[a1]=l[a1]-1;
  return 1;
}

//lab2_challenge2
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
 //sprint("11\n");

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
     pte_t *pte;
    pte= page_walk((pagetable_t)current[(readtp())]->pagetable, g_malloc_page, 1);
    if (*pte & PTE_V)
     {
      user_vm_unmap((pagetable_t)current[(readtp())]->pagetable,g_malloc_page,PGSIZE, 1);
     }
    user_vm_map((pagetable_t)current[(readtp())]->pagetable, g_malloc_page, PGSIZE, (uint64)pa,
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

    user_vm_map((pagetable_t)current[readtp()]->pagetable, g_malloc_page, PGSIZE, (uint64)pa,
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
void do_exec(char *s, char *para)
{
  //elf_info info;
  //sprint("do exec\n");
  char *filename=(char *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), s);
  char *argv=(char *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), para);
  process *p = alloc_process();
  load_bincode_from_host_elf(p, filename);
  //sprint("pid:%d",p->pid);
  //sprint("id:%d\n",current[readtp()]->pid);
  //sprint("p->parent:%d\n",p->parent->pid);
  //sprint("current[readtp()]->parent%d\n",current[readtp()]->parent);
  free_p(current[readtp()]);
  current[readtp()]->kstack = p->kstack;
  current[readtp()]->pagetable = p->pagetable;
  current[readtp()]->trapframe = p->trapframe;
  current[readtp()]->total_mapped_region = p->total_mapped_region;
  current[readtp()]->mapped_info = p->mapped_info;
  current[readtp()]->user_heap = p->user_heap;
  //current[readtp()]->parent = curre->parent;
  current[readtp()]->line=p->line;
  current[readtp()]->file=p->file;
  current[readtp()]->dir=p->dir;
  current[readtp()]->debugline=p->debugline;
  current[readtp()]->queue_next = p->queue_next;
  current[readtp()]->tick_count = p->tick_count;
  current[readtp()]->pfiles = p->pfiles;
  current[readtp()]->pid=p->pid;
  // better malloc
  void *pa = alloc_page();
  uint64 va = current[readtp()]->user_heap.heap_top;
  current[readtp()]->user_heap.heap_top += PGSIZE;
  current[readtp()]->mapped_info[HEAP_SEGMENT].npages++;
  //sprint("1\n");
  user_vm_map((pagetable_t)current[readtp()]->pagetable, va, PGSIZE, (uint64)pa, prot_to_type(PROT_WRITE | PROT_READ, 1));
  //sprint("1\n");
  memset(pa, 0, PGSIZE);

  
  char *pathpa[2];
  uint64 vsp;
  vsp=p->trapframe->regs.sp;
  vsp-=8;
  char * sp=user_va_to_pa((pagetable_t)(p->pagetable),(void *)vsp);
  memcpy((void *)sp,(void *)argv,1+strlen(argv));
  //sprint("sp:%s\n",sp);
  //sprint("vsp:%x\n",vsp);
  //sprint("%x\n",strlen(pa));
  uint64 vsp1=vsp;
  vsp-=8;
  //sprint("%x\n",sp);
  char *sp3=user_va_to_pa((pagetable_t)(p->pagetable),(void *)vsp);
 // sprint("%x\n",sp);
  uint64 *sp2=(uint64 *)sp3;
  //sprint("vsp:%x\n",vsp);
  *sp2=(vsp+8);
  current[readtp()]->trapframe->regs.sp -= 16;
  current[readtp()]->trapframe->regs.a1 = (uint64)vsp;
  current[readtp()]->trapframe->regs.a0 = (uint64)1;

  switch_to(current[readtp()]);
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

ssize_t sys_user_lab1_challenge1(uint64 a1)
{
  //sprint("in syscall\n");
  
  uint64 n=1;
  uint64 i=0; 
  assert(current[readtp()]);
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
  uint64 va=current[readtp()]->trapframe->regs.sp;
  uint64 ar1=0;
//sprint("%x\n",(va));
// for(i=0;i<=20;i++)
// {
//   ar1 = *(uint64 *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), (void *)current[readtp()]->trapframe->regs.sp+i*8);
//   sprint("%d %x\n",i,(ar1));
// }
  va=va+24;
  ar1 = *(uint64 *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), (void *)current[readtp()]->trapframe->regs.sp+3*8);


//  for(i=0;i<=11;i++)
//   sprint("%lx\n",sh[i].offset);
//  sprint("%lx\n",*(uint64 *)(ar1-8));
  while(n<=a1)
  {
    //sprint("11\n");
   for(i=0;sh[i].offset!=1;i++)
    {
      //sprint("11\n");
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
     ar1 = *(uint64 *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable),(void *) va);
     //ar1= *(uint64 *)(ar1-16);
      n+=1;
  }

  return 0;
}


//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current[readtp()] );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}



//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  // sprint("User exit with code:%d.\n", code);
  // // reclaim the current process, and reschedule. added @lab3_1
  // free_process( current[readtp()] );
  // // if(current->pid==0)
  // //   sprint("yes_free\n");
  // schedule(1);
  // return 0; 
  uint64 tp=read_tp();
  sprint("hartid = %d: User exit with code: %d.\n",tp, code);
  sync_barrier(&count_4,NCPU);
  
  if (tp == 0)
  {
    //sprint("1\n");
    //sprint("id:%d\n",current[tp]->pid);
    free_process(current[tp]);
   // sprint("2\n");
    
    schedule(1);
  }
  else
  {
    //sprint("111\n");
    free_process(current[tp]);
    //schedule(1);
  }
  sync_barrier(&count_7,NCPU);
  shutdown(code);
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
  if (current[readtp()]->user_heap.free_pages_count > 0) {
    va =  current[readtp()]->user_heap.free_pages_address[--current[readtp()]->user_heap.free_pages_count];
    assert(va < current[readtp()]->user_heap.heap_top);
  } else {
    // otherwise, allocate a new page (this increases the size of the heap by one page)
    va = current[readtp()]->user_heap.heap_top;
    current[readtp()]->user_heap.heap_top += PGSIZE;

    current[readtp()]->mapped_info[HEAP_SEGMENT].npages++;
  }
  user_vm_map((pagetable_t)current[readtp()]->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current[readtp()]->pagetable, va, PGSIZE, 1);
  // add the reclaimed page to the free page list
  current[readtp()]->user_heap.free_pages_address[current[readtp()]->user_heap.free_pages_count++] = va;
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current[readtp()] );
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
  insert_to_ready_queue(current[readtp()]);
  schedule(0);
  // panic( "You need to implement the yield syscall in lab3_2.\n" );

  return 0;
}

//
// open file
//
ssize_t sys_user_open(char *pathva, int flags) {
  char* pathpa = (char*)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), pathva);
  //sprint("patha:%s\n",pathpa);
  char pathb[30];
  if(*pathpa=='.'&&*(pathpa+1)=='.')
  {
    strcpy(pathb,current[readtp()]->pfiles->cwd->name);
  }
  else if(*pathpa=='.'&&*(pathpa+1)!='.')
  {
    strcpy(pathb,current[readtp()]->pfiles->cwd->name);
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
  else if(*pathpa!='.')
  {
    strcpy(pathb,pathpa);
  }
  
  //sprint("pathb:%s\n",pathb);
  return do_open(pathb, flags);
}

//
// read file
//
ssize_t sys_user_read(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = lookup_pa((pagetable_t)current[readtp()]->pagetable, addr);
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
    uint64 pa = lookup_pa((pagetable_t)current[readtp()]->pagetable, addr);
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
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), istat);
  return do_stat(fd, pistat);
}

//
// read disk inode
//
ssize_t sys_user_disk_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), istat);
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
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), pathva);
  return do_opendir(pathpa);
}

//
// lib call to readdir
//
ssize_t sys_user_readdir(int fd, struct dir *vdir){
  struct dir * pdir = (struct dir *)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), vdir);
  return do_readdir(fd, pdir);
}

//
// lib call to mkdir
//
ssize_t sys_user_mkdir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), pathva);
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
  char * pfn1 = (char*)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), (void*)vfn1);
  char * pfn2 = (char*)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), (void*)vfn2);
  return do_link(pfn1, pfn2);
}

//
// lib call to unlink
//
ssize_t sys_user_unlink(char * vfn){
  char * pfn = (char*)user_va_to_pa((pagetable_t)(current[readtp()]->pagetable), (void*)vfn);
  return do_unlink(pfn);
}

ssize_t sys_user_exec(char *s,char *para)
{
  //sprint("o\n");  //sprint();
  //sprint("%s\n",apathpa);
  /// /bin/app_mkdir
  // int fd=do_open(pathpa,O_RDWR | O_CREAT);
  //process *st=alloc_page();
  do_exec(s,para);
  // //sprint("fd:%d\n",fd);
  // //sprint("0\n");
  return -1;
 
}
ssize_t sys_user_exec1(char *s,char *para)
{
  //sprint("o\n");  //sprint();
  //sprint("%s\n",apathpa);
  /// /bin/app_mkdir
  // int fd=do_open(pathpa,O_RDWR | O_CREAT);
  //process *st=alloc_page();
  syscall_load_bincode_from_host_elf(current[readtp()],s,para);
  // //sprint("fd:%d\n",fd);
  // //sprint("0\n");
  return -1;

}
ssize_t sys_user_ccwd(char *path){
  //char *path1=path;
  char *path1=user_va_to_pa(current[readtp()]->pagetable,path);//查询到当前的进程路径
  if(*path1=='.'&&*(path1+1)=='.')
    strcpy(current[readtp()]->pfiles->cwd->name,"/");
  else if(*path1=='.'&&*(path1+1)=='/')
  {
   //sprint("change1:%s\n",path1);
    strcpy(current[readtp()]->pfiles->cwd->name,path1+1);
    //sprint("change1:%s\n",path);
    //strcpy(user)
  }
  else
    strcpy(current[readtp()]->pfiles->cwd->name,path1);
  return 0;
}
//路径读
ssize_t sys_user_rcwd(char *path){
  
 
  //sprint("read1:%s\n",user_va_to_pa(current->pagetable,path))
  //存在path时要放在实地址
  strcpy((char*)user_va_to_pa(current[readtp()]->pagetable,path),current[readtp()]->pfiles->cwd->name);
  //strcpy((char*)path,current->pfiles->cwd->name);
  return 0;
}
//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//


ssize_t sys_user_gets(char *s){
  uint64 tp=read_tp();
  assert(current[tp]);
 char *pa=user_va_to_pa(current[tp]->pagetable,(void *)s);
 char c;
 while(1)
 {
  c=vgetc(stdin);
  if(c!='\n')
    *pa=c;
  else 
    break;
  pa=pa+1;
 }
 *pa='\0';
  return 0;  
}

ssize_t sys_user_printpa(uint64 va)
{
  //一个虚拟地址的8 9位为rsw未被使用可用来标记是不是写时复制场景
  uint64 tp=read_tp();
  uint64 pa = (uint64)user_va_to_pa((pagetable_t)(current[tp]->pagetable), (void*)va);
  sprint("%lx\n", pa);
  return 0;
}
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
    case SYS_user_sem_new:
      return sys_user_sem_new(a1);
    case SYS_user_sem_P:
      return sys_user_sem_P(a1);
    case SYS_user_sem_V:
      return sys_user_sem_V(a1);
    case SYS_user_printpa:
      return sys_user_printpa(a1);
    case SYS_user_gets:
      return sys_user_gets((char *)a1);
    case SYS_user_exec1:
      return sys_user_exec1((char *)a1,(char*)a2);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
