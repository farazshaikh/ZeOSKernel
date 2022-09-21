/** @file malloc.c
 *  @brief These functions are wrapper functions to memory management calls
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <mutex.h>

/*semaphore used to lock multiple mem calls*/
mutex_t mutex_safe = { 0, 0};

void *malloc(size_t __size)
{
  char *ret_p;
  mutex_lock(&mutex_safe);
  ret_p = _malloc(__size);
  mutex_unlock(&mutex_safe);
  return ret_p;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
  char *ret_p;
  mutex_lock(&mutex_safe);
  ret_p = _calloc(__nelt,__eltsize);
  mutex_unlock(&mutex_safe);
  return ret_p;
}
void *realloc(void *__buf, size_t __new_size)
{
  char *ret_p;
  mutex_lock(&mutex_safe);
  ret_p = _realloc(__buf,__new_size);
  mutex_unlock(&mutex_safe);
  return ret_p;
}

void free(void *__buf)
{

  mutex_lock(&mutex_safe);
  _free(__buf);
  mutex_unlock(&mutex_safe);

}

