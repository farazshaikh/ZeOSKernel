/**@file sc_lc_fork.S
 * @brief stub for  system call - fork
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>

#define THIS_SYSCALL_INT         FORK_INT
#define THIS_SYSCALL_PARAMS_NR   0
#define THIS_SYSCALL_STR         "fork"
#include "sc_asm_template.h"

int fork(void) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  
}

