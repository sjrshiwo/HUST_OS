/*
 * contains the implementation of all syscalls.
 */
#define page_manage_start 0x00000000 + PGSIZE * 1024
#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "spike_interface/spike_utils.h"
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
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page(uint64 a1) {
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
uint64 sys_user_free_page(uint64 va) {
  // user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  
  block_free_va(p,va);
  //sprint("444\n");
  return 0;
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
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page(a1);//a1代表分配空间的大小
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
