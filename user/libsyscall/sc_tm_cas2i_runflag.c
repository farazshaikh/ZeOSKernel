/**@file sc_tm_cas2i_runflag.S
 * @brief stub for  system call - cas2i_runflag
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#include <syscall_int.h>

#define THIS_SYSCALL_INT         CAS2I_RUNFLAG_INT
#define THIS_SYSCALL_PARAMS_NR   6
#define THIS_SYSCALL_STR         "cas2i_runflag"
#include "sc_asm_template.h"

int cas2i_runflag(
		  int tid, 
		  int *oldp, 
		  int ev1, 
		  int nv1, 
		  int ev2, 
		  int nv2) {
  int __result;
  trap_str(THIS_SYSCALL_INT,THIS_SYSCALL_PARAMS_NR);
  return __result;  
}
