/**@file sc_tm_yield.S
 * @brief stub for  system call - yield
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>

#define THIS_SYSCALL_INT         YIELD_INT
#define THIS_SYSCALL_PARAMS_NR   1
#define THIS_SYSCALL_STR         "yield"
#include "sc_asm_template.h"

int yield(int pid) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  
}
