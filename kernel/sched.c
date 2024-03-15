/*
 * implementing the scheduler
 */

#include "sched.h"
#include "spike_interface/spike_utils.h"
process* wait_queue_head= NULL;
process* ready_queue_head = NULL;

//
// insert a process, proc, into the END of ready queue.
//
void insert_to_ready_queue( process* proc ) {
  sprint( "going to insert process %d to ready queue.\n", proc->pid );
  // if the queue is empty in the beginning
  if( ready_queue_head == NULL ){
    proc->status = READY;
    proc->queue_next = NULL;
    ready_queue_head = proc;
    return;
  }

  // ready queue is not empty
  process *p;
  // browse the ready queue to see if proc is already in-queue
  for( p=ready_queue_head; p->queue_next!=NULL; p=p->queue_next )
    if( p == proc ) return;  //already in queue

  // p points to the last element of the ready queue
  if( p==proc ) return;
  p->queue_next = proc;
  proc->status = READY;
  proc->queue_next = NULL;

  return;
}

//
// choose a proc from the ready queue, and put it to run.
// note: schedule() does not take care of previous current process. If the current
// process is still runnable, you should place it into the ready queue (by calling
// ready_queue_insert), and then call schedule().
//
extern process procs[NPROC];
void schedule(int type) {
  if(wait_queue_head!=NULL&&type==1)//等待队列非空
  {
    
    //sprint("1111\n");
    int flag=0;
    process t=*wait_queue_head;    
    process *head=wait_queue_head;
    process *tp;
    process *pro=NULL;
    //sprint("%d\n",current->parent->pid);//子进程号是2
    //sprint("%d\n",t.pid);//第一个父进程号是1
    if(current->parent!=NULL)
    {
      while(1)
      {
        //sprint("111\n");
        if(t.pid==current->parent->pid)
       {
        //sprint("%d\n",t.pid);
        flag=1;
        break;
       }
      if(t.queue_next==NULL) break;
      t=*t.queue_next;
      
      
      }
    }
    //sprint("call\n");
    //问题父进程process0未加入到等待队列 去看插入的代码
    //sprint("%d\n",flag);
   //以上正确
    if(flag==1)
    {
      //sprint("111\n");
      for(;;)
      {
        if(wait_queue_head->pid==t.pid)
        {
           
          if(pro==NULL)
          {
            
            wait_queue_head=wait_queue_head->queue_next;
            
            break;
          }
          else 
          {
            tp=wait_queue_head->queue_next;
            pro->queue_next=tp;
            wait_queue_head=head;
          }
          break;
        }
        wait_queue_head=wait_queue_head->queue_next;
        if(pro==NULL)
          pro=head;
        else 
        {
          pro=pro->queue_next;
        }  
      }
      
  //     int should_shutdown = 1;

  //   for( int i=0; i<NPROC; i++ )
  //     if( (procs[i].status != FREE) && (procs[i].status != ZOMBIE) ){
  //       should_shutdown = 0;
  //       sprint( "ready queue empty, but process %d is not in free/zombie state:%d\n", 
  //         i, procs[i].status );
  //     }

  //   if( should_shutdown ){
  //     sprint( "no more ready processes, system shutdown now.\n" );
  //     shutdown( 0 );
  //   }else{
  //     panic( "Not handled: we should let system wait for unfinished processes.\n" );
    
  // }
      //sprint("22\n"); 
      current=&t;
      current->status = RUNNING;
      sprint("going to insert process %d to ready queue.\n",current->pid );
      sprint( "going to schedule process %d to run.\n", current->pid );
      switch_to( current );
      
    }
  }

  else if ( !ready_queue_head ){
    // by default, if there are no ready process, and all processes are in the status of
    // FREE and ZOMBIE, we should shutdown the emulated RISC-V machine.
     //sprint("2222\n");
    
    int should_shutdown = 1;
    //sprint("no wait\n");
    for( int i=0; i<NPROC; i++ )
      if( (procs[i].status != FREE) && (procs[i].status != ZOMBIE) ){
        
        should_shutdown = 0;
        //sprint("%d\n",procs[i].pid);
        sprint( "ready queue empty, but process %d is not in free/zombie state:%d\n", 
          i, procs[i].status );
      }

    if( should_shutdown ){
      sprint( "no more ready processes, system shutdown now.\n" );
      shutdown( 0 );
    }else{
      panic( "Not handled: we should let system wait for unfinished processes.\n" );
    }
  }

  current = ready_queue_head;
  assert( current->status == READY );
  ready_queue_head = ready_queue_head->queue_next;

  current->status = RUNNING;
  sprint( "going to schedule process %d to run.\n", current->pid );
  switch_to( current );
}

void insert_to_wait_queue(process *proc)//等待队列
{

  if(wait_queue_head==NULL)
  {
    proc->status = BLOCKED;
    proc->queue_next = NULL;
    wait_queue_head = proc;
    return;
  }
  process *p;
  // browse the ready queue to see if proc is already in-queue
  for( p=wait_queue_head; p->queue_next!=NULL; p=p->queue_next )
    if( p == proc ) return;  //already in queue

  // p points to the last element of the ready queue
  if( p==proc ) return;
  p->queue_next = proc;
  proc->status =  BLOCKED;
  proc->queue_next = NULL;
  return;
}