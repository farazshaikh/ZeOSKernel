/**@file sc_lc_wait.S
 * @brief stub for  system call - wait
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>

#define THIS_SYSCALL_INT         WAIT_INT
#define THIS_SYSCALL_PARAMS_NR   1
#define THIS_SYSCALL_STR         "wait"
#include "sc_asm_template.h"

int wait(int *status_ptr) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result; 
}
