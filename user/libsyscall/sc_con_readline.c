/**@file sc_con_readline.S
 * @brief stub for  system call - readline
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         READLINE_INT
#define THIS_SYSCALL_PARAMS_NR   2
#define THIS_SYSCALL_STR         "readline_pages"
#include "sc_asm_template.h"

int readline(int size, char *buf) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  //- handle special no-return case -//
}
