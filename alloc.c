/**
 * Machine Problem: Malloc
 * CS 241 - Fall 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>


typedef struct freeBlock{
  struct freeBlock* prev;
  struct freeBlock* next;
} freeBlock;

static void* tail;
static void* firstAlloc;
static size_t memAvailable;
static freeBlock* front;
static size_t MEM_FOR_FREE = 16 * ((2 * sizeof(size_t) + 1 + sizeof(freeBlock) + 15) / 16);
int splitBlock(void*, size_t);

size_t totalMemNeeded(size_t size){
  size_t memNeeded = 16 * ((size + 1 + 2 * sizeof(size_t) + 15) / 16);
  return memNeeded < MEM_FOR_FREE ? MEM_FOR_FREE : memNeeded;
}

void writeSizeToTags(void* ptr, size_t blockSize, char available){
  size_t memNeeded = totalMemNeeded(blockSize);
  *(size_t*)ptr = blockSize;
  *(char*)((size_t*)ptr + 1) = available;
  *(size_t*)((char*)ptr + memNeeded - sizeof(size_t)) = blockSize;
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    void* ptr = malloc(num*size);
    if(!ptr){
      return NULL;
    }
    return memset(ptr, 0, num * size);
}

void* freshMemory(size_t size){
  size_t memNeeded = totalMemNeeded(size);

  void* temp;
  if(memAvailable >= memNeeded){
    memAvailable -= memNeeded;
  }
  else{
    temp = sbrk(memNeeded - memAvailable);
    if(!temp){
      return 0;
    }
    memAvailable = 0;
  }

  void* newPtr;
  if(!tail){
    newPtr = temp;
  }
  else{
    newPtr = (char*)tail + totalMemNeeded(*(size_t*)tail);
  }

  if(!firstAlloc){
    firstAlloc = newPtr;
  }
  writeSizeToTags(newPtr, size, 0);
  tail = newPtr;

  return (char*)newPtr + sizeof(size_t) + 1;

}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    //TODO set available field is used elsewhere
    // implement malloc!
    // new memory
    if(!tail){
      return freshMemory(size);
    }

    freeBlock* i = front;
    while(i){
      size_t blockSize = *(size_t*)((char*)i - 1 - sizeof(size_t));
      if(blockSize >= size){
        if(i->next){
          i->next->prev = i->prev;
        }
        if(i == front){
          front = front->next;
        }
        else if(i->prev){
          i->prev->next = i->next;
        }
        if((char*)i - 1 - sizeof(size_t) == tail){
          memAvailable += totalMemNeeded(blockSize) - totalMemNeeded(size);
          blockSize = size;
        }
        writeSizeToTags((char*)i - 1 - sizeof(size_t), blockSize, 0);
        if((char*)i - 1 - sizeof(size_t) != tail && blockSize > size){
          splitBlock((char*)i - 1 - sizeof(size_t), size);
        }
        return i;
      }
      i = i->next;
    }

    return freshMemory(size);
}

int mergeBlocks(void* ptr1, void* ptr2, int firstInList, int secondInList){
  if(!ptr1 || !ptr2 || ptr1 == tail){
    return 0;
  }
  if(!*((char*)ptr1 + sizeof(size_t)) || !*((char*)ptr2 + sizeof(size_t))){
    return 0;
  }

  size_t sizeOne = *(size_t*)ptr1;
  size_t sizeTwo = *(size_t*)ptr2;
  void* nextToOne = (char*)ptr1 + totalMemNeeded(sizeOne);
  if(ptr2 == nextToOne){
    size_t newBlockSize = totalMemNeeded(sizeOne) + totalMemNeeded(sizeTwo) - 2 * sizeof(size_t) - 1;
    freeBlock* one = (freeBlock*)((char*)ptr1 + 1 + sizeof(size_t));
    freeBlock* two = (freeBlock*)((char*)ptr2 + 1 + sizeof(size_t));
    writeSizeToTags(ptr1, newBlockSize, 1);
    if(ptr2 == tail){
      tail = ptr1;
    }
    if(firstInList && !secondInList){
      return 1;
    }
    one->next = two->next;
    if(two->next){
      two->next->prev = one;
    }
    if(!firstInList){
      one->prev = two->prev;
      if(two->prev){
        two->prev->next = one;
      }
    }
    if(front == two){
      front = one;
    }
    return 1;
  }
  return 0;
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    if(!ptr){
      return;
    }

    void* block = (char*)ptr - sizeof(size_t) - 1;
    size_t size = *(size_t*)block;
    if(block == tail){
      size += memAvailable;
      memAvailable = 0;
    }
    writeSizeToTags(block, size, 1);
    freeBlock* freeSeg = (freeBlock*)ptr;
    freeSeg->next = 0;
    freeSeg->prev = 0;

    if(!front){
      front = freeSeg;
      return;
    }

    void* prevBlock = 0;
    if(block != firstAlloc){
      size_t prevSize = *(size_t*)((char*)block - sizeof(size_t));
      prevBlock = (char*)block - totalMemNeeded(prevSize);
    }

    void* nextBlock = 0;
    if(block != tail){
      nextBlock = (char*)block + totalMemNeeded(size);
    }

    int firstMerge = mergeBlocks(block, nextBlock, 0, 1);
    int secondMerge = mergeBlocks(prevBlock, block, 1, firstMerge);
    if(firstMerge || secondMerge){
      return;
    }


    freeBlock* i = front;
    freeBlock* prev = 0;
    while(i){
      if(freeSeg < i){
        break;
      }
      prev = i;
      i = i->next;
    }

    if(!prev){
      freeSeg->next = front;
      front->prev = freeSeg;
      front = freeSeg;
      return;
    } else{
      prev->next = freeSeg;
      freeSeg->prev = prev;
    }

    freeSeg->next = i;
    if(i){
      i->prev = freeSeg;
    }

}

int splitBlock(void*ptr, size_t blockSize){
  size_t currentSize = *(size_t*)ptr;
  if(blockSize > currentSize){
    return 0;
  }
  if(totalMemNeeded(currentSize) - totalMemNeeded(blockSize) <= MEM_FOR_FREE){
    return 0;
  }

  writeSizeToTags(ptr, blockSize, 0);
  void* newBlock = (char*)ptr + totalMemNeeded(blockSize);
  size_t newBlockSize = totalMemNeeded(currentSize) - totalMemNeeded(blockSize) - 1 - 2 * sizeof(size_t);

  if(tail == ptr){
    tail = newBlock;
  }

  *(size_t*)newBlock = newBlockSize;
  free((char*)newBlock + 1 + sizeof(size_t));
  return 1;
}



/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    if(!ptr){
      return malloc(size);
    }
    else if(!size){
      free(ptr);
      return 0;
    }

    void* block = (char*)ptr - sizeof(size_t) - 1;
    size_t blockSize = *(size_t*)block;
    if(blockSize >= size){
      if(blockSize > size){
        if(block == tail){
          writeSizeToTags(block, size, 0);
          memAvailable += totalMemNeeded(blockSize) - totalMemNeeded(size);
        } else{
          splitBlock(block, size);
        }
      }
      return ptr;
    }

    if(block == tail){
      size_t currentSize = totalMemNeeded(blockSize);
      size_t newSize = totalMemNeeded(size);
      if(memAvailable >= newSize - currentSize){
          memAvailable -= (newSize - currentSize);
      } else{
        sbrk(newSize - currentSize - memAvailable);
        memAvailable = 0;
      }
      writeSizeToTags(block, size, 0);
      return ptr;
    }

    void* nextBlock = (char*)block + totalMemNeeded(blockSize);
    if(*((char*)nextBlock + sizeof(size_t))){
      size_t nextSize = *(size_t*)nextBlock;
      size_t combinedSize = totalMemNeeded(blockSize) + totalMemNeeded(nextSize) - 1 - 2 * sizeof(size_t);
      if(combinedSize >= size){
        freeBlock* tmp = (freeBlock*)((char*)nextBlock + 1 + sizeof(size_t));
        if(nextBlock == tail){
          tail = block;
          memAvailable += totalMemNeeded(combinedSize) - totalMemNeeded(size);
          combinedSize = size;
        } else if(tmp->next){
          tmp->next->prev = tmp->prev;
        }
        if(tmp == front){
          front = front->next;
        } else if(tmp->prev){
          tmp->prev->next = tmp->next;
        }
        writeSizeToTags(block, combinedSize, 0);
        if(block != tail && combinedSize > size){
          splitBlock(block, size);
        }
        return ptr;
      }
    }

    void* newPtr = malloc(size);
    memcpy(newPtr, ptr, blockSize);
    free(ptr);
    return newPtr;
}
