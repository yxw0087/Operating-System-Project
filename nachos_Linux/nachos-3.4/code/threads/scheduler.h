// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();			// Initialize list of ready threads 
    ~Scheduler();			// De-allocate ready list

    void ReadyToRun(Thread* thread);	// Thread can be dispatched.
    void RoundRobin(Thread* thread);
    void setQuantum();
    int getQuantum(Thread* thread);
    int get_num_process() { return num_process; }
    int get_remaining_process() { return remaining_process; }
    void decrement_process() { remaining_process--; }
    void method_analysis();
    void time_copy(int which, int waitingTime, int completeTime);
    void wait();
    Thread* FindNextToRun();		// Dequeue first thread on the ready 
					// list, if any, and return thread.
    void Run(Thread* nextThread);	// Cause nextThread to start running
    void Print();			// Print contents of ready list
    
  private:
    List *readyList;  		// queue of threads that are ready to run,
				// but not running
    int waitingTime[26];
    int completeTime[26];
    int quantum;
    int num_process;
    int remaining_process;
};

#endif // SCHEDULER_H
