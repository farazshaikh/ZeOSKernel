/** @file     kern_err.h
 *  @brief    This file exports the definitions 
 *            of the globally used kernel error codes
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#ifndef _KERN_ERR_H
#define _KERN_ERR_H

typedef int KERN_RET_CODE;
#define KERN_SUCCESS                0 
#define KERN_ERROR_GENERIC         -1
#define KERN_ERROR_UNIMPLEMENTED   -2 
#define KERN_NO_MEM                -4
#define KERN_ERROR_CURSOR_FADANGO  -5

#define KERN_ERROR_INVALID_SYSCALL -6

#define KERN_NOT_AN_ELF             -7
#define KERN_ERROR_VM_CANNOT_MAP    -8
#define KERN_ERROR_FILE_NOT_FOUND   -9
#define KERN_ERROR_TASK_NOT_FOUND   -10
#define KERN_BUFFER_TOO_SMALL       -11

#define KERN_ERROR_ADDRESS_NOT_PRESENT -12
#define KERN_PAGE_ERR                -13
#define KERN_ERR_BAD_SYS_PARAM      -14
            
#endif
 
