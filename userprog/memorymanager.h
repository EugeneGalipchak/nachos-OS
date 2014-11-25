#include "copyright.h"
#include "bitmap.h"
#include "synch.h"

class MemoryManager {
  public:
    MemoryManager(int numPages);
    ~MemoryManager();

    int AllocPage();                // Allocate clear page
    void FreePage(int physPageNum); // Frees physical page
    int getAvailable();
    bool PageIsAllocated(int physPageNum);

  private:
    BitMap *pages;                  // Size of PHY_PAGES
    int Num_Pages;
    int usedPagesTotal;             // Numnber of the used pages
    Lock* lock;                     // Thread Lock
};
