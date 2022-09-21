/**@file sc_asm_template.h
 * @brief template header file for subsequently declared templates.
 *        Every system call is implemented as a wrapper to the asm code in this library.
 *
 * @author Deepak Amin (dvamin), Faraz Shaikh (fshaikh)
 *
 * @bug None known
 */

#ifndef SYSCALL_INTERNAL_H 
#define SYSCALL_INTERNAL_H 
 
#ifndef THIS_SYSCALL_INT
#error THIS_SYSCALL_INT definition required for ASM wrapper for system call
#endif

#ifndef THIS_SYSCALL_PARAMS_NR
#error THIS_SYSCALL_PARAMS_NR definition required for ASM wrapper for system call
#endif

#ifndef THIS_SYSCALL_STR
#error THIS_SYSCALL_STR definition required for ASM wrapper for system call
#endif


#define _trap_instruction(intno)					\
  "push %%ebx;\n\t"							\
  "push %%ecx;\n\t"							\
  "push %%edx;\n\t"							\
  "push %%ebp;\n\t"							\
  "push %%esi;\n\t"							\
  "push %%edi;\n\t"							\
  "int $" #intno ";\n\t"						\
  "pop %%edi;\n\t"							\
  "pop %%esi;\n\t"							\
  "pop %%ebp;\n\t"							\
  "pop %%edx;\n\t"							\
  "pop %%ecx;\n\t"							\
  "pop %%ebx;\n\t"							



#define _trap_head 	__asm__ (		

#define _trap_str_0(SYSCALL_NR)						\
"movl $" #SYSCALL_NR ",%%ecx;\n\t"                                      \
"movl %%ecx,%%edx;\n\t"							

#define _trap_str_1(SYSCALL_INT,SYSCALL_NR) 			        \
"cp_params"#SYSCALL_INT":\n\t"						\
"movl 4(%%ebp,%%ecx,4),%%eax;\n\t"					\
"pushl 4(%%ebp,%%ecx,4);\n\t"						\
"decl %%ecx;\n\t"							\

#define _trap_str_2(SYSCALL_INT,SYSCALL_NR)                             \
"jnz cp_params" #SYSCALL_INT ";\n\t"					\
"mov  %%esp,%%esi;\n\t"                                                 \
_trap_instruction(SYSCALL_INT)					        \
"leal  (%%esp,%%edx,4),%%esp;\n\t"						

#define _trap_tail                                                      \
: "=a" (__result)							\
:									\
:"%ecx","%esi","%edx")




//- We have 3 case here                                              --//
//- 0 parameter system call                                          --//
//- 1 parameter system call                                          --//
//- >1 parameter system call                                         --//

#if THIS_SYSCALL_PARAMS_NR==0 

#define trap_str(SYSCALL_INT,SYSCALL_NR)	                        \
   _trap_head                                                           \
   _trap_instruction(SYSCALL_INT)					\
   _trap_tail


#elif THIS_SYSCALL_PARAMS_NR==1

#define trap_str(SYSCALL_INT,SYSCALL_NR)	                        \
   _trap_head                                                           \
   "movl 8(%%ebp),%%esi;\n\t"		                                \
   _trap_instruction(SYSCALL_INT)					\
   _trap_tail


#else

#define trap_str(SYSCALL_INT,SYSCALL_NR)	\
   _trap_head					\
   _trap_str_0(SYSCALL_NR)			\
   _trap_str_1(SYSCALL_INT,SYSCALL_NR)	        \
   _trap_str_2(SYSCALL_INT,SYSCALL_NR)	        \
   _trap_tail 
#endif


#endif //-- SYSCALL_INTERNAL_H 
