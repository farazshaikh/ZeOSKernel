/**@file sc_lc_task_vanish.S
 * @brief stub for  system call - task_vanish
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         TASK_VANISH_INT
#define THIS_SYSCALL_PARAMS_NR   1
#define THIS_SYSCALL_STR         "task vanish"
#include "sc_asm_template.h"

void task_vanish(int status) { 
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  while(1);
}
