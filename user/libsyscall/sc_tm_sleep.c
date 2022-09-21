/**@file sc_tm_sleep.c
 * @brief stub for  system call - sleep
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>

#define THIS_SYSCALL_INT         SLEEP_INT
#define THIS_SYSCALL_PARAMS_NR   1
#define THIS_SYSCALL_STR         "sleep"
#include "sc_asm_template.h"

int sleep(int ticks) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  
}
