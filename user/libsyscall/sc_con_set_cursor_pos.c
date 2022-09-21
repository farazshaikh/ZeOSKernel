/**@file sc_con_set_cursor_pos.S
 * @brief stub for  system call - set_cursor_pos
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>
#include <syscall.h>
#include <stdlib.h>

#define THIS_SYSCALL_INT         SET_CURSOR_POS_INT
#define THIS_SYSCALL_PARAMS_NR   2
#define THIS_SYSCALL_STR         "set_cursor_pos"
#include "sc_asm_template.h"

int set_cursor_pos(int row, int col) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;
}
