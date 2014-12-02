// threadtest.cc
// Simple test case for the threads assignment.
//
// Create two threads, and have them context switch
// back and forth between themselves by calling Thread::Yield,
// to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved. See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// Loop 5 times, yielding the CPU to another ready thread
// each iteration.
//
// "which" is simply a number identifying the thread, for debugging
// purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
  int num;

  for (num = 0; num < 5; num++) {
    printf("*** thread %d looped %d times\n", which, num);
    currentThread->Yield();
  }
}

//----------------------------------------------------------------------
// ThreadTest1
// Set up a ping-pong between two threads, by forking a thread
// to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
  DEBUG('t', "Entering ThreadTest1");

  Thread *t = new Thread("forked thread");

  t->Fork(SimpleThread, 1);
  SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest
// Invoke a test routine.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// LockTest1
//----------------------------------------------------------------------

Lock *locktest1 = NULL;      //tests for locks
Lock *locktestDel = NULL;
Condition *condi;            //condi variable
Mailbox * mailbox = new Mailbox("Mailbox"); //mailbox tests
Whale * whale = new Whale("Whale");         //whale tests

void
LockThread1(int param)
{
  locktestDel = new Lock("LockTestDel"); //TEST 3
  delete locktestDel;
  ASSERT(locktestDel);

  printf("L1:0\n");
  //locktest1->Release(); //TEST 2
  locktest1->Acquire();
  //locktest1->Acquire(); //TEST 1
  printf("L1:1\n");
  currentThread->Yield();
  printf("L1:2\n");
  locktest1->Release();
  printf("L1:3\n");
}

void
LockThread2(int param)
{
  printf("L2:0\n");
  locktest1->Acquire();
  printf("L2:1\n");
  currentThread->Yield();
  printf("L2:2\n");
  locktest1->Release();
  printf("L2:3\n");
}

void
LockTest1()
{
  DEBUG('t', "Entering LockTest1");

  locktest1 = new Lock("LockTest1");

  Thread *t = new Thread("one");
  t->Fork(LockThread1, 0);
  t = new Thread("two");
  t->Fork(LockThread2, 0);
}

//PART 1 TESTING

void acquireTwice() { // acquiring twice
  locktest1 = new Lock("locktest1");

  locktest1->Acquire();
  printf("acquiring twice\n");
  locktest1->Acquire();
}

void acquireNoLock() { //acquiring with no lock
  locktest1 = new Lock("locktest1");

  printf("Acquiring no lock!\n");
  locktest1->Release();
}

void deleteLock() {   //deleting lock not held
  locktest1 = new Lock("locktest1");

  locktest1->Acquire();
  printf("deleting held lock\n");
  delete locktest1;
}

void conditionWait() {//waiting on lock not held
  locktest1 = new Lock("locktest1");
  Condition *wait;
  wait =  new Condition("wait");  
 
  printf("waiting on condition variable with no lock!\n");
  wait->Wait(locktest1); 
}

void signalNbroadcast() {
  locktest1 = new Lock("locktest1");
  Condition *wait;
  wait =  new Condition("wait");  

  locktest1->Acquire();
  wait->Signal(locktest1);
    printf("signaled success!\n");
  
  locktest1->Acquire();
  wait->Broadcast(locktest1);
    printf("boardcast success!\n");
}
                      //signal holds lock
void SignalHoldsLock() {
  locktest1 = new Lock("locktest1");
  Condition *wait;
  wait =  new Condition("wait");  

  wait->Signal(locktest1);
}
                      //no threads after deleting waiting in queue
void noThread() {
  locktest1 = new Lock("locktest1");

  delete locktest1;
  printf("successfully deleted with no threads in queue\n");
} 

//PART 2 TESTING

int * buffer;      //initializing buffer and message
int message = 0;

void sender(int param) { //creating send method for test
  mailbox->Send(message);
}

void receiver(int param) { //creating receive method for test
  mailbox->Receive(buffer);
}

void senderSends() {  //BASIC CALLS TEST
  Thread * t;         //initializing new thread

  t = new Thread("sender");
 
  message = 1;        //setting data for message
 
  t->Fork(sender, 0); //calling fork

  currentThread->Yield(); //yeilding thread
 
  t = new Thread("receiver");
  
                      //MULTIPLE RECEIVERS/SENDERS TEST
  t->Fork(receiver, 0);

  currentThread->Yield();

  t = new Thread("receiver");
 
  t->Fork(receiver, 0);
  
  currentThread->Yield();
 
  t = new Thread("sender");
  
  message = 3;

  t->Fork(sender, 0);
  
  currentThread->Yield();
  
  t = new Thread("sender");
  
  message = 4;

  t->Fork(sender, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");
  
  message = 5;

  t->Fork(sender, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");
  
  message = 6;

  t->Fork(sender, 0); 
  
  currentThread->Yield();
}

//----------------------------------------------------------------------
// Testing Part 4 PRIORITY 
//----------------------------------------------------------------------
//objects
Lock * lock;
Semaphore * semaphore;
Condition * cond;
//give each thread something to do, prints out the assigned priority
//to each created thread
void echo( int parameter )
{
   printf("Thread has priority number %d\n ", parameter);
}
void echoLock( int parameter )
{
   lock->Acquire();
   
   printf("Thread has priority number %d\n ", parameter);
   lock->Release();
}
void echoSemaphore(int parameter)
{
    semaphore->P();
     
    printf("Thread has priority number %d\n ", parameter);
      
    semaphore->V();
}
void echoCond(int parameter)
{
   lock->Acquire();
   
   cond->Wait(lock);
}
//check to see if priority works
void testingPriority()
{
   Thread * thread; //threads
  
   //create 5 threads and assign priority number to it
   for( int i = 0; i < 5; i++)
   {
      thread = new Thread("thread");
      //assign priority number to each thread
      thread->setPriority(i);
      //fork threads
      thread->Fork(echo, i);
      
      //tell user what they should expect 
      printf("Thread has priority number %d is forked\n", i);
   }
   
   printf("print statements should be printed out in descending order\n");
}
//----------------------------------------------------------------------
// Testing priority with locks
//----------------------------------------------------------------------
void testingPriorityLocks()
{
   //created three threads 
   Thread * babyThread1 =new Thread("babyThread1");
   Thread * babyThread2 =new Thread("babyThread2");
   Thread * babyThread3 =new Thread("babyThread3");
   lock = new Lock("babyLocks");
   //give each thread a priority
   babyThread1->setPriority(1);
   babyThread2->setPriority(2);
   babyThread3->setPriority(3);
   
   lock->Acquire();
   babyThread1->Fork(echoLock, 1);
   babyThread2->Fork(echoLock, 2);
   babyThread3->Fork(echoLock, 3);
   currentThread->Yield();
   printf("print statements should be printed out in descending order\n");
   lock->Release();
}
//----------------------------------------------------------------------
// Testing priority with semaphores
//----------------------------------------------------------------------
void testingPrioritySemaphores()
{
   //created three threads 
   Thread * babyThread1 =new Thread("babyThread1");
   Thread * babyThread2 =new Thread("babyThread2");
   Thread * babyThread3 =new Thread("babyThread3");
   //init obj
   semaphore = new Semaphore("semaphore", 0);
   //give each thread a priority
   babyThread1->setPriority(1);
   babyThread2->setPriority(2);
   babyThread3->setPriority(3);
   
   semaphore->P();
   babyThread1->Fork(echoSemaphore, 1);
   babyThread2->Fork(echoSemaphore, 2);
   babyThread3->Fork(echoSemaphore, 3);
   currentThread->Yield();
   printf("print statements should be printed out in descending order\n");
   semaphore->V();
}
//----------------------------------------------------------------------
// Testing priority with Condition Variables
//----------------------------------------------------------------------
void testingPriorityCondition()
{
   //created three threads 
   Thread * babyThread1 =new Thread("babyThread1");
   Thread * babyThread2 =new Thread("babyThread2");
   Thread * babyThread3 =new Thread("babyThread3");
   //init obj
   cond = new Condition("condition");
   lock = new Lock("Lock");
   
   //give each thread a priority
   babyThread1->setPriority(1);
   babyThread2->setPriority(2);
   babyThread3->setPriority(3);
   
   
   babyThread1->Fork(echoCond, 1);
   babyThread2->Fork(echoCond, 2);
   babyThread3->Fork(echoCond, 3);
   printf("print statements should be printed out in descending order\n");
   for( int i = 0; i < 3; i++)
   {
      lock->Acquire();
      cond->Signal(lock);
      lock->Release();
      currentThread->Yield();
   }
   currentThread->Yield();
   
}

//PART 5 TESTING

void male(int param) { //creating send method for test
  whale->Male();
}

void female(int param) { //creating receive method for test
  whale->Female();
}

void match(int param) { //creating receive method for test
  whale->Matchmaker();
}

void whaleSend() {  //BASIC MATING TEST
  Thread * t;         //initializing new thread

  t = new Thread("sender");
 
  t->Fork(male, 0); //calling fork

  currentThread->Yield(); //yeilding thread
 
  t = new Thread("receiver");
                      //MULTIPLE RECEIVERS/SENDERS TEST
  t->Fork(female, 0);

  currentThread->Yield();

  t = new Thread("receiver");
 
  t->Fork(match, 0);
  
  currentThread->Yield();
                      //DOUBLE CALL TEST 
  t = new Thread("sender");
 
  t->Fork(male, 0); //calling fork

  currentThread->Yield(); //yeilding thread
 
  t = new Thread("receiver");
                      //MULTIPLE RECEIVERS/SENDERS TEST
  t->Fork(female, 0);

  currentThread->Yield();

  t = new Thread("receiver");
 
  t->Fork(match, 0);
 
                      //MULTILE MALE/FEMALE TEST 
  currentThread->Yield();

  t = new Thread("sender");
  
  t->Fork(male, 0);
  
  currentThread->Yield();
  
  t = new Thread("sender");
  
  t->Fork(female, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");
  
  t->Fork(male, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");

  t->Fork(female, 0); 
  
  currentThread->Yield(); 
  
  t = new Thread("receiver");
 
  t->Fork(match, 0);
  
                    //MULTIPLE MALE/MATCH TEST
  currentThread->Yield();

  t = new Thread("sender");
  
  t->Fork(male, 0);
  
  currentThread->Yield();
  
  t = new Thread("sender");
  
  t->Fork(match, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");
  
  t->Fork(male, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");

  t->Fork(match, 0); 
  
  currentThread->Yield(); 
  
  t = new Thread("receiver");
 
  t->Fork(female, 0);

                    //MULTIPLE MATCH/FEMALE TEST
  currentThread->Yield();

  t = new Thread("sender");
  
  t->Fork(match, 0);
  
  currentThread->Yield();
  
  t = new Thread("sender");
  
  t->Fork(female, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");
  
  t->Fork(match, 0); 
  
  currentThread->Yield();

  t = new Thread("sender");

  t->Fork(female, 0); 
  
  currentThread->Yield(); 
  
  t = new Thread("receiver");
 
  t->Fork(male, 0);
}

void
Joiner(Thread *joinee)
{
  //currentThread->Yield();
  currentThread->Yield();

  printf("Waiting for the Joinee to finish executing.\n");
  printf("1\n");
  currentThread->Yield();
  printf("2\n");
  currentThread->Yield();
  printf("3\n");
  // Note that, in this program, the "joinee" has not finished
  // when the "joiner" calls Join().  You will also need to handle
  // and test for the case when the "joinee" _has_ finished when
  // the "joiner" calls Join().
  

  joinee->Join();
  printf("4\n");
  currentThread->Yield();
  printf("5\n");
  currentThread->Yield();
  printf("6\n");
  printf("Joinee has finished executing, we can continue.\n");

  currentThread->Yield();
  printf("7\n");
  currentThread->Yield();
  printf("8\n");
  joinee->Join();

}

void
Joinee()
{
  int i;

  for (i = 0; i < 7; i++) {
    printf("Smell the roses.%d\n", i);
    currentThread->Yield();
  }

  currentThread->Yield();
  printf("Done smelling the roses!\n");
  currentThread->Yield();
}

/* Part 3 Testing */
void
ForkerThread()
{
  Thread *joiner = new Thread("joiner", 0);  // will not be joined
  Thread *joinee = new Thread("joinee", 1);  // WILL be joined

  // fork off the two threads and let them do their business
  joiner->Fork((VoidFunctionPtr) Joiner, (int) joinee);
  joinee->Fork((VoidFunctionPtr) Joinee, 0);

  //Thread *joiner_2 = new Thread("hello_world", 0);  // will not be joined
  //Thread *joinee_2 = new Thread("hello_world", 0);  // WILL be joined

  //joiner_2->Fork((VoidFunctionPtr) Joiner, (int) joinee);
  //joinee_2->Fork((VoidFunctionPtr) Joinee, 0);
  // this thread is done and can go on its merry way
  printf("Forked off the joiner and joiner threads.\n");
}


//----------------------------------------------------------------------
// ThreadTest
// Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
  {
  switch (testnum) {
    case 1:
    ThreadTest1();
    break;
    case 2:
    LockTest1();
    break;
    case 3:
    acquireTwice();
    break;
    case 4:
    acquireNoLock();
    break;
    case 5:
    deleteLock();
    break;
    case 6:
    conditionWait();
    break;
    case 7:
    signalNbroadcast();
    break;
    case 8:
    SignalHoldsLock();
    break;
    case 9:
    noThread();
    break;
    case 10:
    senderSends();
    break;
    case 11:
    whaleSend();
    break;
    
    // part 3
    case 12:
    ForkerThread();
    break;
    // part 4 priority
    case 13:
    testingPriorityCondition();
    break;
    case 14:
    testingPrioritySemaphores();
    break;
    case 15:
    testingPriorityLocks();
    break;
    case 16:
    testingPriority();
    break;   
    default:
    printf("No test specified.\n");
    break;
  }
}
