/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H


typedef struct mutex {
  volatile int is_locked;
  int thread_id;             //- Should be treated as a hint -//
} mutex_t;

/******Helper Function Declaration*******/
int is_mutex_locked( mutex_t *mp );

#endif /* _MUTEX_TYPE_H */
