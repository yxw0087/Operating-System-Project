// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <math.h>
#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List;
    quantum = 0;
    num_process = 0;
    remaining_process = 0;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list based on method chosen.\n", thread->getName());

    thread->setStatus(READY);
    
    if(method == 0) {
        readyList->Append((void *)thread); // Normal RR
    }
    else if(method == 1) {
        readyList->SortedInsert((void *)thread, thread->getPriority()/thread->getTime()); 
	// My modified RR
    }
    else if(method == 2) {
        readyList->SortedInsert((void *)thread, thread->getTime()); 
	// Other people's modified RR
    }
    num_process++;
    remaining_process++;
}

void
Scheduler::RoundRobin (Thread *thread)
{
    DEBUG('t', "Putting thread %s at the end of ready list.\n", thread->getName());

    thread->setStatus(READY);
    readyList->Append((void *)thread); // Normal RR
}

void
Scheduler::setQuantum ()
{
    int i = 0;
    int total_time = 0;
    float max_burst = 0; //only used to implement method 2(from other people) for comparison
    while(readyList->getAt(i)) {
	Thread *t = (Thread *)readyList->getAt(i);
	total_time += t->getTime();
	i++;
	max_burst = max(max_burst, t->getTime());
    }
    float mean_time = (float) total_time/i;
//printf("max burst is: %f", mean_time);//sqrt(mean_time*max_burst));
    if(method == 2) { quantum = floor(sqrt(max_burst*mean_time)); }
    else { quantum = sqrt(mean_time); }
}

int
Scheduler::getQuantum (Thread *thread)
{
    if(method == 0) { return quantum; }
    else if(method == 1) {
        if(thread->getTime() >= quantum) {
	    float q_ratio = (float) (thread->getTime() - quantum)/quantum;	    
	    if(q_ratio <= 0.5) {
	        return quantum + (thread->getTime() - quantum);
	    }
	    return quantum;	    
        }
        else {
	    return thread->getTime();
        }
    }
    else if(method == 2) { return quantum; }
}

void
Scheduler::method_analysis ()
{
    int total_waiting = 0;
    int total_complete = 0;

    for(int i = 0; i < num_process; i++)
    {
	total_waiting += waitingTime[i];
	total_complete += completeTime[i];
    }
    
    float average_waiting = (float)total_waiting/num_process;
    float average_complete =  (float)total_complete/num_process;
    
    printf("\n\nAverage waiting time is: %f", average_waiting);
    printf("\nAverage turnaround time is: %f", average_complete);
    printf("\nNumber of context switches: %d\n\n", context_switch);
}

void
Scheduler::time_copy (int which, int waiting, int complete)
{
    waitingTime[which] = waiting;
    completeTime[which] = complete;
}

void
Scheduler::wait ()
{
    for(int i = 0; i < remaining_process - 1; i++)
    {
	Thread *t = (Thread *)readyList->getAt(i);
	t->increase_wait();
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    return (Thread *)readyList->Remove();
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}
