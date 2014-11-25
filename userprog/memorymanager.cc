#include "memorymanager.h"

//////////////////////////////////////////////////////////////////////
//   Create a manager to track the allocation of numPages. Will create
//   one by calling the constructor with NumPhysPages in the parameter.
//   All physical pages start as free, unallocated. 
//////////////////////////////////////////////////////////////////////
MemoryManager::MemoryManager(int numPages) {
  Num_Pages = numPages;
  pages = new BitMap(Num_Pages); 
  lock = new Lock("lock");
}

//////////////////////////////////////////////////////////////////////
//      Delete bitmap and lock
//////////////////////////////////////////////////////////////////////
MemoryManager::~MemoryManager () {
  delete pages;
  delete lock;
}

//////////////////////////////////////////////////////////////////////
//   Allocate a free page, returning its physical page number or if there 
//   are no free pages available.
//////////////////////////////////////////////////////////////////////
int MemoryManager::AllocPage () {
  lock->Acquire(); //acquire lock

  int free = pages->Find();      //return index of page with find()

  if(free == -1)                 //check if pages are in use
    return -1;

  usedPagesTotal++;
      
  lock->Release(); //release lock

  return free;                   //return found page
}

//////////////////////////////////////////////////////////////////////
//   Takes the index of page and frees the physical page for future
//   allocation.
//////////////////////////////////////////////////////////////////////
void MemoryManager::FreePage (int physPageNum) {
  lock->Acquire(); //acquire lock
      
  pages->Clear(physPageNum);

  usedPagesTotal--;
      
  lock->Release(); //release lock
}

bool MemoryManager::PageIsAllocated(int physPageNum){
  lock->Acquire(); //acquire lock

  int alloced = pages->Test(physPageNum);

  lock->Release(); //release lock

  return alloced;
}

//////////////////////////////////////////////////////////////////////
//   Return the number of clear bits in the bitmap.
//  (In other words, how many bits are unallocated?) 
//////////////////////////////////////////////////////////////////////
int MemoryManager::getAvailable () {
  lock->Acquire(); //acquire lock
  
  int tmp = pages->NumClear();
  
  lock->Release(); //release lock
  return tmp;    
}
