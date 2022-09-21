/** @file     kern_debug.h
 *  @brief    This file contains the interfaces for the kernel debugger 
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include <simics.h>

//#define KERNEL_DEBUG 1 
#define ENTRY_EXITS  1 

#ifdef KERNEL_DEBUG


/** @macros DEBUG_PRINT
 *  @brief  dumps output to simics when debugging is enabled
 */

#define DEBUG_PRINT(fmt,args...) do {           \
    lprintf((fmt),##args);	                \
  } while(0)



/** @macros FN_ENTRY
 *  @brief  dumps function entry when debuggin is enabled
 */

#define FN_ENTRY() do{				\
    lprintf("ENTRY:%s", __FUNCTION__);		\
  }while(0);



/** @macros FN_LEAVE
 *  @brief  dumps function exit when debuggin is enabled
 */

#define FN_LEAVE() do {				\
    lprintf("EXIT:%s", __FUNCTION__);		\
  }while(0)

#else 

#define DEBUG_PRINT(fmr,arg...)
#define FN_ENTRY()
#define FN_LEAVE()

#endif 



/** @macros DUMP
 *  @brief  Always dumps to simics irrespective of debug/release build
 */

#define DUMP(fmt,args...) do {			\
    lprintf((fmt),##args);	                \
  } while(0)



/** @macros C_ASSERT
 *  @brief  Compile time assert
 */

#define C_ASSERT(exp) do {			\
    int arr[(exp)?1:-1];			\
    arr[0] = 0;					\
    arr[0] = arr[0];				\
} while(0)

#endif // _KERNEL_DEBUG_H


