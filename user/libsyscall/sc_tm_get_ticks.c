/**@file sc_tm_get_ticks.c
 * @brief stub for  system call - get_ticks
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>

#define THIS_SYSCALL_INT         GET_TICKS_INT
#define THIS_SYSCALL_PARAMS_NR   0
#define THIS_SYSCALL_STR         "get_ticks"
#include "sc_asm_template.h"

int get_ticks() {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  
}
