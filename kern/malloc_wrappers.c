/** @file     malloc_wrappers.c
 *  @brief    This file contains the thread-safe calls to memory allocation calls
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <stddef.h>
#include <malloc.h>
#include <malloc/malloc_internal.h>
#include <simics.h>
#include <kern_common.h>
#include <string.h>

//#define DEBUG_ALLOCS

#ifdef DEBUG_ALLOCS
#define DUMP_MEM lprintf
#else
#define DUMP_MEM(fmt,args...)
#endif

semaphore malloc_mutex;
/* safe versions of malloc functions */
void *malloc(size_t size)
{
  void *ptr;
  sem_wait(&malloc_mutex);
  ptr = _malloc(size);
  sem_signal(&malloc_mutex);
  DUMP_MEM("malloc %p",ptr);
  return ptr;
}

void *memalign(size_t alignment, size_t size)
{
  return NULL;
}

void *calloc(size_t nelt, size_t eltsize)
{
  return NULL;
}

void *realloc(void *buf, size_t new_size)
{
  return NULL;
}

void free(void *buf)
{
  DUMP_MEM("free %p",buf);
  sem_wait(&malloc_mutex);
  _free(buf);
  sem_signal(&malloc_mutex);
  return;
}

void *smalloc(size_t size)
{
  return NULL;
}

void *smemalign(size_t alignment, size_t size)
{
  void *ptr;
  sem_wait(&malloc_mutex);
  ptr = _smemalign(alignment,size);
  sem_signal(&malloc_mutex);
  DUMP_MEM("smemalign %p %d",ptr,size);
  return ptr;
}

void *scalloc(size_t size)
{
  return NULL;
}

void sfree(void *buf, size_t size)
{
  DUMP_MEM("sfree %p %d",buf,size);
  sem_wait(&malloc_mutex);
  _sfree(buf,size);
  sem_signal(&malloc_mutex);
  return;
}


void malloc_init() {
  SEMAPHORE_INIT(&malloc_mutex,1);
}
