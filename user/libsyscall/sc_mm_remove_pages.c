/**@file sc_mm_removepages.c
 * @brief stub for  system call - removepages
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         REMOVE_PAGES_INT
#define THIS_SYSCALL_PARAMS_NR   1
#define THIS_SYSCALL_STR         "remove_pages"
#include "sc_asm_template.h"

int remove_pages(void * addr) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  
}
