/**@file sc_con_get_cursor_pos.S
 * @brief stub for  system call - get_cursor_pos
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         GET_CURSOR_POS_INT
#define THIS_SYSCALL_PARAMS_NR   2
#define THIS_SYSCALL_STR         "get_cursor_pos"
#include "sc_asm_template.h"

int get_cursor_pos(int *row, int *col) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;
}
