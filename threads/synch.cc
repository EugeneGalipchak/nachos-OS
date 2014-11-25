// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    while (value == 0) { 			// semaphore not available
        queue->SortedInsert((void *)currentThread, ( -1 * currentThread -> getPriority()));	// so go to sleep
        currentThread->Sleep();
    }
    value--; 					// semaphore available,
    // consume its value

    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
        scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments
// Note -- without a correct implementation of Condition::Wait(),
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {
  name = debugName;
  queue = new List; 
  myThread = NULL;
  value = 0;
}

Lock::~Lock() {
  ASSERT(!(isHeldByCurrentThread()));       // TEST 3
  ASSERT(value == 0);                        //assertion for value;
  delete queue;
  ASSERT(queue);                            //assertion for empty queue
}

bool Lock::isHeldByCurrentThread() {
    return(currentThread == myThread);      //is current thread held?
}

void Lock::Acquire() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    printf("DEBUG: in Acquire()\n");
    
    ASSERT(!(isHeldByCurrentThread()));       // TEST 1

    while (value == 1) { 			                // lock not available
        queue->SortedInsert((void *)currentThread, ( -1 * currentThread  -> getPriority()));	// so go to sleep
        //queue->Append ((void*) currentThread);
        currentThread->Sleep();
    }
    value = 1; 					                      // lock available,

    myThread = currentThread;                 //set threads equal to eachother
    (void) interrupt->SetLevel(oldLevel);	    // re-enable interrupts
}

void Lock::Release() {
    printf("DEBUG: in Release()\n");
    
    ASSERT(!(myThread == NULL));              // TEST 2
    ASSERT((isHeldByCurrentThread()));          // current thread test

    Thread *thread;                           // creating new thread
    IntStatus oldLevel = interrupt->SetLevel(IntOff); //intterupt off

    thread = (Thread *)queue->Remove();       //remove from front of queue

    if (thread != NULL)	                      // make thread ready
        scheduler->ReadyToRun(thread);

    value = 0;                                //set val back to 0 
    myThread = NULL;                          
    (void) interrupt->SetLevel(oldLevel);     //re-enable interrupt
}

Condition::Condition(char* debugName) {
  name = debugName;
  queue = new List; 
  myThread = NULL; 
}

Condition::~Condition() { 
  delete queue;
  ASSERT(queue);
}

//----------------------------------------------------------------------
// Condition::Wait
// Atomically releases the lock and suspends execution of the calling 
// thread, placing the calling thread on the condition variables waiting 
// queue. Later, when the calling thread is reenabled, it reacquires the 
// lock before returning from the wait() call.
// 
// Input Parameters : Pointer to the lock which needs to wait
// Return Parameters : Nothing, void.
//----------------------------------------------------------------------
void Condition::Wait(Lock* conditionLock) {
  if(conditionLock->isHeldByCurrentThread()) { //initial check for lock
    IntStatus oldLevel = interrupt->SetLevel(IntOff); //turn off inter

    queue->SortedInsert((void *)currentThread, ( -1 * currentThread  -> getPriority()));
    conditionLock->Release();     //release lock
    currentThread->Sleep();       //sleep thread
    conditionLock->Acquire();     //acquire the lock

    (void) interrupt->SetLevel(oldLevel);             //turn on inter
  }
}

//----------------------------------------------------------------------
// Condition::Signal
// Takes one waiting thread off the condition variables waiting queue and 
// marks it as eligible to run (i.e., it puts the thread on the schedulers
// ready list.)
//
// Input Parameters : Pointer to the lock which needs to be waken-up
// Return Parameters : Nothing, void.
//----------------------------------------------------------------------
void Condition::Signal(Lock* conditionLock) { 
  if(conditionLock->isHeldByCurrentThread()) { //initial check for lock

    IntStatus oldLevel = interrupt->SetLevel(IntOff); //turn off inter

    if(!queue->IsEmpty())                      //if queue isnt emtpy then R2R
      scheduler->ReadyToRun((Thread *)queue->Remove());

    (void) interrupt->SetLevel(oldLevel);             //turn on inter
  }
}

//----------------------------------------------------------------------
// Condition::Broadcast
// Takes all waiting threads off the condition variables waiting queue 
// and marks them as eligible to run.
//
// Input Parameters : Pointer to the lock which needs to waken-up
// Return Parameters : Nothing, void.
//----------------------------------------------------------------------
void Condition::Broadcast(Lock* conditionLock) { 
  if(conditionLock->isHeldByCurrentThread()) {    //initial check for lock

    IntStatus oldLevel = interrupt->SetLevel(IntOff); //turn off inter
  
    while(!queue->IsEmpty())               //if queue is emptry then R2R
      scheduler->ReadyToRun((Thread *)queue->Remove());
  

    (void) interrupt->SetLevel(oldLevel);  //turn on inter
  }
}

// --------------------------------------------------------------------
// PART 2
// --------------------------------------------------------------------
Mailbox::Mailbox(char *debugName) {
    name = debugName;
    mailLock =     new Lock("lock buffer");
    sendWait =     new Condition("waitCond");
    receiveWait =  new Condition("signalCond");
}

Mailbox::~Mailbox() {
    delete mailLock;
    delete sendWait;
    delete receiveWait;
}

void Mailbox::Send(int message) {
    mailLock->Acquire();           //first stage of send/receive
                                   //trap waiting for Receive()
    while(receiveBool < 1 || copied == 1) 
      sendWait->Wait(mailLock);            //keeps waiting until receiveBool++

    buff = message;                //copy message into buffer

    copied = 1;                    //copied set to true

    printf("Sent!\n");
    receiveWait->Signal(mailLock); 

    mailLock->Release();
}

void Mailbox::Receive(int * message) {      
    mailLock->Acquire();           //first stage of send/receive

    receiveBool++;                 //boolean used to actuate Send()

    sendWait->Signal(mailLock);            //start by signaling

    while(copied != 1)             //wait until message is copied
      receiveWait->Wait(mailLock);         //into the buffer

    printf("Received!\n");
    *message = buff;               //message pointer == buffer
    copied = 0;                    //set copy back to zero
    receiveBool--;                 //decrement so Send() will wait

    mailLock->Release();
}

//-------------------------------------------------------------------
// PART 5
// ----------------------------------------------------------------
Whale::Whale(char *debugName) {
    name = debugName;
    whaleLock =  new Lock("whaleLock");
    maleWait =   new Condition("maleWait");
    femaleWait = new Condition("femaleWait");
    matchWait =  new Condition("matchWait");
}

Whale::~Whale() {
    delete whaleLock;
    delete maleWait;
    delete femaleWait;
    delete matchWait;
}

void Whale::Male() {
    whaleLock->Acquire();          //first stage of send/receive
    maleBool++;         
                                   //trap waiting for Receive()
                                   //   ) && malePres == 0)
    while(femaleBool < 1 || matchBool < 1 || malePres == 1)
      maleWait->Wait(whaleLock);   //keeps waiting until receiveBool++

    malePres = 1;
                      //Check to make sure that matches are same
    if(malePres == 1 && femalePres == 1 && matchPres == 1) {
      maleBool--;    //if true, then reset all bool values.
      femaleBool--;
      matchBool--;
      malePres = 0;
      femalePres = 0;
      matchPres = 0;   
    }
                      //Signal all
    printf("Whales mating\n");
    maleWait->Signal(whaleLock);
    femaleWait->Signal(whaleLock);
    matchWait->Signal(whaleLock);
    
    whaleLock->Release();
}

void Whale::Female() {
    whaleLock->Acquire();          //first stage of send/receive
    femaleBool++; 
                                   //trap waiting for Receive()
    while(maleBool < 1 || matchBool < 1 || femalePres == 1)
      femaleWait->Wait(whaleLock); //keeps waiting until receiveBool++

    femalePres = 1;

                      //Check to make sure that matches are same
    if(malePres == 1 && femalePres == 1 && matchPres == 1) {
      maleBool--;     //if true, then reset all bool values.
      femaleBool--;
      matchBool--;
      malePres = 0;
      femalePres = 0;
      matchPres = 0;
    }
                      //Signal all
    printf("Whales mating\n");
    maleWait->Signal(whaleLock);
    femaleWait->Signal(whaleLock);
    matchWait->Signal(whaleLock);

    whaleLock->Release();
}

void Whale::Matchmaker() {
    whaleLock->Acquire();          //first stage of send/receive
    matchBool++; 
                                   //trap waiting for Receive()
    while(maleBool < 1 || femaleBool < 1 || matchPres == 1)
      matchWait->Wait(whaleLock);  //keeps waiting until receiveBool++

    matchPres = 1;
                      //Check to make sure that matches are same
    if(malePres == 1 && femalePres == 1 && matchPres == 1) {
      maleBool--;     //if true, then reset all bool values
      femaleBool--;
      matchBool--;
      malePres = 0;
      femalePres = 0;
      matchPres = 0;
    }
                      //Signal all
    printf("Whales mating\n");
    maleWait->Signal(whaleLock);
    femaleWait->Signal(whaleLock);
    matchWait->Signal(whaleLock);

    whaleLock->Release();
}
