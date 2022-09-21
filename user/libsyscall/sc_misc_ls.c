/**@file sc_misc_ls.c
 * @brief stub for  system call - ls
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         LS_INT
#define THIS_SYSCALL_PARAMS_NR   2
#define THIS_SYSCALL_STR         "ls"
#include "sc_asm_template.h"

int ls(int size, char *buf) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;
}
