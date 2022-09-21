/**@file sc_misc_halt.S
 * @brief stub for system call - halt
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         HALT_INT
#define THIS_SYSCALL_PARAMS_NR   0
#define THIS_SYSCALL_STR         "halt"
#include "sc_asm_template.h"

void halt() {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return;
}
