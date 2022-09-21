/**@file sc_tm_getid
 * @brief stub for  system call - get_thrid()
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>

#define THIS_SYSCALL_INT         GETTID_INT
#define THIS_SYSCALL_PARAMS_NR   0
#define THIS_SYSCALL_STR         "gettid"
#include "sc_asm_template.h"

int gettid() {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  
}
