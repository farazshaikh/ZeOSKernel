/**@file sc_con_print.S
 * @brief stub for  system call - print
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         PRINT_INT
#define THIS_SYSCALL_PARAMS_NR   2
#define THIS_SYSCALL_STR         "print"
#include "sc_asm_template.h"

int print(int size, char *buf) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;
}
