// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "table.h"

extern Table * table;
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

int Table::Table(int size)
{   
   //make it thread safe
   lock = new Lock("TABLE LOCK");

   //array of untyped object pointers indexed by integers in range of 
   //0 to size-1

   tableSize = size;
   pointer = new void*[size];
   
   for( int i=0; i<size; i++)
   {
      pointer[i] = 0;
   }
   
}

void exitInternal(int status)
{
  
   printf(status);

   //
   for( int i= 0; i < 128; i++)
   {
      
      if(currentThread->space == table->Get(i))
      {
         table->Release(i);
         break;
      }
   }

   delete currentThread->space;
   currentThread->Finish();
}

int Table::Alloc(void *object)
{
   lock->Acquire();

   for(int i=0; i < tableSize; i++ )
   {
      if(pointer[i] == 0)
      {
         pointer[i] = object; //fill empty slot with object
          
         lock->Release();
         return i;
      }
   }
   lock->Release();

   return -1; //return -1 if no free slots are available. 
}

void *Table::Get(int index)
{
   lock->Acquire();
   
   //if outside of index range, then return null
   if( index < 0 || index >= tableSize ) 
   {
      lock->Release();
      return NULL;
   }
  
   void *foundPointer = pointer[index];

   lock->Release();
   return foundPointer;

}

void Table::Release(int index)
{
   lock->Acquire();

   //check if outside range
   if( index < 0 || index >= tableSize ) 
   {
      lock->Release();
      return NULL;
   }
   pointer[index] = 0;

   lock->Release();
}

void poop()
{
   //runs the program
   currentThread->InitRegisters(); //set the inital register values
   currentThread->RestoreState(); //load page table register

   machine->Run(); //jump to the user program
  
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    }
 
    else if ((which == SyscallException) && ( type == SC_Exec))
    {
         DEBUG('a', "Exec called.\n");
         //read name of file located in reg 4 as stated above
         char * fileName = machine->ReadRegister(4);
         char buffer;
         char * inFile = new char[100];
         
         //iterate through each char to check if valid. 
         for( i=0; i<100; i++ )
         {
            //valid string if ends with null because every string 
            //ends with \0
            if( Machine->readMem( ((int)fileName+i), 1, &buffer) )
            {
               kernalArray[i] = buffer;
               if( buffer != '\0' )
               {
                  break;
               }
            }
            //otherwise return 0 to indicate an error
            else  
            {
               //return 0 and write into register 2
               machine->WriteRegister(2, 0);
            }
         }
         //check if last char is not null then its not valid, return 0 
         if( i = 100 && inFile[99] != '\0'  )
         {
            machine->WriteRegister(2, 0);
         }
        
         OpenFile *executable = fileSystem->Open(filename);
         AddrSpace * space;
      
         if(executable == NULL)
         {
            printf("Unable to open file %s\n", filename);
            machine->WriteRegister(2, 0);
         } 
         space = new AddrSpace(executable);

         delete executable; //close file
                 
         Thread * thread = new Thread("STRING FOR DEBUGGING PURPOSES");

         thread->space = space;  
         
         //address space exits
         //by doing syscall "exit"
         
         //return unique process identifier (SpaceId) for
         // each process created. 
         int procID= table->Alloc(space);

         if(procID == -1  )
         {
           //delete space and thread if Alloc returns -1
           //which indicates that there are no free spots available
           delete space;
           delete thread;
           machine->WriteRegister(2, 0);

         }
         
         thread->Fork(poop, 0);
   
         machine->WriteRegister(2, procID);
    } 

    else if ((which == SyscallException) && (type == SC_Exit))
    {
      DEBUG('a', "Exit clled.\n");

      //void Exit(int status)
      int status = machine->ReadRegister(4);
      
      exitInternal(status);

    }
      

    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }

 
     //increment pc counter

}
