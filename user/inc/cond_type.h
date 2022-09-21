/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <mutex.h>
#include <wait_control_block.h>


#define SIGNALLED 1
#define BLOCKED   0

typedef struct cond {
  WAIT_CONTROL_BLOCK condWaitControl;
} cond_t;

/**Initialization Macro**/
#define INIT_COND_VAR( cvar )  do {			\
    INIT_WAIT_CONTROL_BLCK (&(cvar)->condWaitControl);	\
}while(0)

#endif /* _COND_TYPE_H */
