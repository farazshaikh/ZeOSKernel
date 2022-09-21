/**@file sc_con_get_cursor_pos.S
 * @brief stub for  system call - sc_con_get_cursor_pos
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         GETCHAR_INT
#define THIS_SYSCALL_PARAMS_NR   0
#define THIS_SYSCALL_STR         "getchar"
#include "sc_asm_template.h"

char getchar(void) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  //- handle special no-return case -//
}
