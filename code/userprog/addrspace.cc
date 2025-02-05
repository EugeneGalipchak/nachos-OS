// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "memorymanager.h"

#ifdef HOST_SPARC
#include <strings.h>

#endif

extern MemoryManager * memManager;

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

  static void
SwapHeader (NoffHeader *noffH)
{
  noffH->noffMagic = WordToHost(noffH->noffMagic);
  noffH->code.size = WordToHost(noffH->code.size);
  noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
  noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
  noffH->initData.size = WordToHost(noffH->initData.size);
  noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
  noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
  noffH->uninitData.size = WordToHost(noffH->uninitData.size);
  noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
  noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------
bool AddrSpace::Initialize(OpenFile *executable) {
  NoffHeader noffH;
  unsigned int i, size;

  executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
  if ((noffH.noffMagic != NOFFMAGIC) &&
      (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    SwapHeader(&noffH);
  ASSERT(noffH.noffMagic == NOFFMAGIC);

  // how big is address space?
  size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
    + UserStackSize;	// we need to increase the size
  // to leave room for the stack
  numPages = divRoundUp(size, PageSize);
  size = numPages * PageSize;

  ASSERT(numPages <= NumPhysPages);		// check we're not trying
  // to run anything too big --
  // at least until we have
  // virtual memory

  DEBUG('a', "Initializing address space, num pages %d, size %d\n",
      numPages, size);

  //check for memory
  if(((int)numPages) > (memManager->getAvailable())) {
    printf("ERROR(pre setting up translation): Ran out of Memory!\n");

    return false; //works after we move this code into addrspace::initialize
  }

  // first, set up the translation
  pageTable = new TranslationEntry[numPages];
  for (i = 0; i < numPages; i++) {
    pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
    pageTable[i].physicalPage = memManager->AllocPage();
    pageTable[i].valid = TRUE;
    pageTable[i].use = FALSE;
    pageTable[i].dirty = FALSE;
    pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
    // a separate page, we could set its
    // pages to be read-only
  }


  ///////////////OLD CODE////////////////
  // zero out the entire address space, to zero the unitialized data segment
  // and the stack segment
  //    bzero(machine->mainMemory, size);
  // fill with zeros the entire uninitialized user address space

  //Get the pointer to the physical page by mapping the virtual page to the
  //physical page, zero te physical page using the pointer to it.
  for(i = 0; i < numPages; i++) {
    memset(&(machine->mainMemory[pageTable[i].physicalPage*PageSize]),0, 
        sizeof(PageSize));
  }

  unsigned int Coffset, Doffset;                                                          
  unsigned int virt_pageNum; 
  int VirtAddr = 0;     //virtual address calculation

  /////////////////////////////////////CODE////////////////////////////////////////////
  if (noffH.code.size > 0) {                                                    
    DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",                 
        noffH.code.virtualAddr, noffH.code.size);                               
                                                                                
    Coffset = (unsigned) noffH.code.virtualAddr % PageSize;                      
    virt_pageNum = (unsigned) noffH.code.virtualAddr / PageSize;                         
 
    VirtAddr = pageTable[virt_pageNum].physicalPage * PageSize + Coffset;
                                                 
    executable->ReadAt(&(machine->mainMemory[Read(VirtAddr)]), PageSize - 
      Coffset, noffH.code.inFileAddr);                              
 
   int Code_readSize = PageSize - Coffset;

    while((Code_readSize + PageSize) <= noffH.code.size) {                           
      virt_pageNum++;                                                                    
      
      VirtAddr = pageTable[virt_pageNum].physicalPage * PageSize;
                                                                          
      executable->ReadAt(&(machine->mainMemory[Read(VirtAddr)]), 
        PageSize, noffH.code.inFileAddr + Code_readSize);                          
                                                                                
      Code_readSize += PageSize;                                                     
    }                                                                           
                                                                                
    if(Code_readSize < noffH.code.size) {                                            
      virt_pageNum++;   

      VirtAddr = pageTable[virt_pageNum].physicalPage * PageSize;
                                                                 
      executable->ReadAt(&(machine->mainMemory[Read(VirtAddr)]),
       noffH.code.size - Code_readSize, noffH.code.inFileAddr + Code_readSize);        
    }                                                                           
  }                                                                             
 
 ///////////////////////////////////////DATA//////////////////////////////////////////                                                                               
  if(noffH.initData.size > 0) {                                                
    DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",                 
        noffH.initData.virtualAddr, noffH.initData.size);                       
                                                                                
    Doffset = (unsigned) noffH.initData.virtualAddr % PageSize;                  
    virt_pageNum = (unsigned) noffH.initData.virtualAddr / PageSize;                                                        
    VirtAddr = pageTable[virt_pageNum].physicalPage * PageSize + Doffset;
  
    executable->ReadAt(&(machine->mainMemory[Read(VirtAddr)]),
       PageSize - Doffset, noffH.initData.inFileAddr);                          
  
    int Data_readSize = PageSize - Doffset;
                                               
    while((Data_readSize + PageSize) <= noffH.initData.size) {                       
      virt_pageNum++;                                                                    
    
      VirtAddr = pageTable[virt_pageNum].physicalPage * PageSize;      
                                                                            
      executable->ReadAt(&(machine->mainMemory[Read(VirtAddr)]),
       PageSize, noffH.initData.inFileAddr + Data_readSize);                      
                                                                                
      Data_readSize += PageSize;                                                     
    }                                                                           
                                                                                
    if(Data_readSize < noffH.initData.size) {                                        
      virt_pageNum++;                                                                    

      VirtAddr = pageTable[virt_pageNum].physicalPage * PageSize;    

      executable->ReadAt(&(machine->mainMemory[Read(VirtAddr)]), 
        noffH.initData.size - Data_readSize, noffH.initData.inFileAddr + 
        Data_readSize);
    }                                                                           
  }                                                                             
  printf("done\n");
  return true;
}

//empty ctr
AddrSpace::AddrSpace(OpenFile *executable) {}

int AddrSpace::Read(int i) {
  printf("HALP\n");
  int phys_addr;    //return found physical address
  unsigned virt_addr;
  unsigned file_off;
  unsigned size = PageSize;

  //unsigned to hold very large numbers
  virt_addr = (unsigned)(virt_addr / size);
  file_off = (unsigned)(virt_addr % size);
  phys_addr = pageTable[virt_addr].physicalPage * size + file_off;
  
  return phys_addr; //return computation of physical address
}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
  /*for (unsigned int i = 0; i < numPages; i++){
    memManager->FreePage(pageTable[i].physicalPage);
  }*/
  delete [] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

  void
AddrSpace::InitRegisters()
{
  int i;

  for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

  // Initial program counter -- must be location of "Start"
  machine->WriteRegister(PCReg, 0);

  // Need to also tell MIPS where next instruction is, because
  // of branch delay possibility
  machine->WriteRegister(NextPCReg, 4);

  // Set the stack register to the end of the address space, where we
  // allocated the stack; but subtract off a bit, to make sure we don't
  // accidentally reference off the end!
  machine->WriteRegister(StackReg, numPages * PageSize - 16);
  DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;
}
