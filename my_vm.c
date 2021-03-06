#include "my_vm.h"



/*
Function responsible for allocating and setting your physical memory 
*/
pde_t *physicalMemory;
pde_t *pagedirectory;
struct bitMap physicalBitMap[];
struct bitMap virtualBitMap[];

void SetPhysicalMem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    //We will simulate a physical memory space of 4MB
    physicalMemory = malloc(MAX_MEMSIZE/PGSIZE);

    //We declare a physical bitmap to correspond to physical memory pages
    physicalBitMap[MAX_MEMSIZE / PGSIZE];
    //Allocating a 1 value to indicate freeness
    for (int i = 0; i < (MAX_MEMSIZE/PGSIZE); i++) {
        physicalBitMap[i].free = 1;
    }

    //We allocate page directory
    pagedirectory = malloc(MAX_MEMSIZE);

    //We declare a virtual bitmap to correspond to virtual memory pages
    virtualBitMap[MAX_MEMSIZE / PGSIZE];
    //Allocating a 1 value to indicate freeness
    for (int i = 0; i < (MAX_MEMSIZE/PGSIZE); i++) {
        virtualBitMap[i].free = 1;
    }

    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * Translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

    //masks used to get the indexes and offset necessary to find the physical address
    unsigned pdeMASK = ((1 << 10) - 1) << 22;
    unsigned pteMASK = ((1 << 10) - 1) << 12;
    unsigned offsetMASK = (1 << 12) - 1;

    unsigned* vaAddress = va;

    //indexes of page directory, page table, and index
    unsigned pdeindex = (pdeMASK & *vaAddress) >> 22;
    unsigned pteindex = (pteMASK & *vaAddress) >> 12;
    unsigned offsetIndex = offsetMASK & *vaAddress;
    
    //If translation not successfull
    if (virtualBitMap[pdeindex * 1024 + pteindex].free == 0) {
        return NULL;
    }
    
    //obtains the physical address by adding the page directory index and page table index to the input
    pte_t out = pgdir[pdeindex][&pteindex] + offsetIndex;
    return out;

}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
PageMap(pde_t *pgdir, void *va, void *pa)
{
    /*HINT: Similar to Translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    //masks used to get the indexes and offset 
    unsigned pdeMASK = ((1 << 10) - 1) << 22;
    unsigned pteMASK = ((1 << 10) - 1) << 12;
    unsigned offsetMASK = (1 << 12) - 1;

    unsigned* vaAddress = va;

    //indexes of page directory, page table, and index
    unsigned pdeindex = (pdeMASK & *vaAddress) >> 22;
    unsigned pteindex = (pteMASK & *vaAddress) >> 12;
    unsigned offsetIndex = offsetMASK & *vaAddress;

    //observes if there is mapping at the current index or not by checking the bit map
    if (virtualBitMap[pdeindex * 1024 + pteindex].free == 0) {
        return -1;
    }

    //sets the value at the index value equal to the physical address
    pagedirectory[pdeindex][&pteindex]= pa;
    //sets the virtual address as filled in
    virtualBitMap[pdeindex * 1024 + pteindex].free = 0;
    return 0;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {

    //loops through the bitmap to find a free page
    for (int i = 0; i < length(virtualBitMap); i++) {
        //if we find a free page, we return a pointer to its address
        if (virtualBitMap[i].free == 1) {
            unsigned pteindex = i % 1024;
            return &pagedirectory[i / 1024][&pteindex];
        }
    }
 
    //Use virtual address bitmap to find the next free page
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *myalloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will 
   have to mark which physical pages are used. */

    //We first calculate the number of pages we need to allocate. Each page is 4096 bytes
    int numPages = (num_bytes-1)/4096 + 1;

    //We will create an array of page pointers in order to store free physical memory locations
    void* physicalPages[] = malloc(numPages * sizeof(void*));

    //We will then go through the physical page bitmap to determine if there are enough pages available
    int freePageCount = 0;
    for (int i = 0; i < (MAX_MEMSIZE/PGSIZE); i++) {
        if (physicalBitMap[i].free == 1) {
            unsigned pteindex = i % 1024;
            physicalPages[freePageCount] = &physicalMemory[i / 1024][&pteindex];
            freePageCount++;
            if (freePageCount == numPages) {
                break;
            }
        }
    }

    if (freePageCount < numPages) {
        return NULL;
    }

    //We will create a pointer to the virtual address of the page that has enough free space
    void *freePage[] = malloc(numPages * sizeof(void*));

    //We will go through the virtual bitmap to see if there is a contiguous region of 
    //virtual memory big enough to store numPages amount of pages
    int start = 0;
    int freeVPageCount = 0;
    for (int i = 0; i < (MAX_MEMSIZE/PGSIZE); i++) {
        if (virtualBitMap[i].free == 1) {
            start = 1;
            if (freeVPageCount >= numPages) {
                unsigned pteindex = i % 1024;
                freePage[i] = &pagedirectory[i / 1024][&pteindex];
                break;
            }
            freeVPageCount++;
        } else {
            if (start == 1) {
                freeVPageCount = 0;
            }
        }
    }

    //We will now map the virtual addresses to physical addresses in our Page Table
    for (int i = 0; i < numPages; i++) {
        PageMap(pagedirectory, freePage[i], physicalPages[i]);
    }



    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */


}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void MatMult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to 
    getting the values from two matrices, you will perform multiplication and 
    store the result to the "answer array"*/

       
}
