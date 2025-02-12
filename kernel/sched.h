#ifndef _SCHED_H_
#define _SCHED_H_

#include "process.h"

//length of a time slice, in number of ticks
#define TIME_SLICE_LEN  2

void insert_to_ready_queue( process* proc );
void schedule(int type);
void insert_to_wait_queue(process* proc);
void insert_to_ready_queue_head( process* proc );
void wake_wait_queue(int a1);
#endif
