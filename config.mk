###########################################################################
# This is the include file for the make file.
###########################################################################
# You should have to edit only this file to get things to build.
#
CFLAGS += -g

###########################################################################
# The method for acquiring project updates.
###########################################################################
# This should be "afs" for any Andrew machine, "web" for non-andrew machines
# and "offline" for machines with no network access.
#
# "offline" is strongly not recommended as you may miss important project
# updates.
#
UPDATE_METHOD = offline



###########################################################################
# Every component of the kernel goes into its respective directory
###########################################################################
BOOT_DRVLIB_DIR = bootdrvlib
I386_UTIL_DIR = i386lib
SYSCALL_DIR   = syscall
VMM_DIR = vmm
TASK_DIR = ps
SCHED_DIR = sched
FAULT_DIR = faulthandlers

###########################################################################
# WARNING: Do not put extraneous test programs into the REQPROGS variables.
#          Doing so will put your grader in a bad mood which is likely to
#          make him or her less forgiving for other issues.
###########################################################################

###########################################################################
# Mandatory programs whose source is provided by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN
#
# The idle process is a really good thing to keep here.
#
410REQPROGS = idle init shell

###########################################################################
# Mandatory programs whose source is provided by you
###########################################################################
# A list of the programs in user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN
#
# Leave this blank unless you are writing custom init/idle/shell programs
# (not generally recommended) or temporarily using tweaked versions for
# debugging purposes (in which case please blank this before turning in).
#
STUDENTREQPROGS =

###########################################################################
# WARNING: When we test your code, the two TESTS variables below will be
# ignored.  Your kernel MUST RUN WITHOUT THEM.
###########################################################################

###########################################################################
# Test programs provided by course staff you wish to run
###########################################################################
# A list of the test programs you want compiled in from the 410user/progs
# directory
#
410TESTS = deschedule_hang \
        exec_basic \
	exec_basic_helper \
	fork_test1 \
	fork_wait \
	getpid_test1 \
	halt_test \
	loader_test1 \
	mem_eat_test \
	print_basic \
	readline_basic \
	remove_pages_test1 \
	sleep_test1 \
	stack_test1 \
	wait_getpid \
	wild_test1 \
	yield_desc_mkrun \
	exec_nonexist \
	fork_bomb \
	fork_exit_bomb \
	fork_wait_bomb \
	loader_test2 \
	make_crash \
	make_crash_helper \
	mem_permissions \
	minclone_mem \
	new_pages \
	register_test \
	remove_pages_test2 \
	cho \
	cho2 \
	mandelbrot \
	racer


#exec_basic_helper getpid_test1 peon merchant fork_test1 halt_test \
	fork_exit_bomb cho sleep_test1 wait_getpid wild_test1 exec_basic \
	print_basic minclone_mem mandelbrot agility_drill fork_wait fork_wait_bomb \
	join_specific_test knife make_crash make_crash_helper loader_test1 \
	loader_test2 yield_desc_mkrun deschedule_hang remove_pages_test1 new_pages \
	mem_eat_test mem_permissions remove_pages_test2 slaughter stack_test1

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory
#
STUDENTTESTS =

###########################################################################
# Object files for your thread library
###########################################################################
THREAD_OBJS = malloc.o 	\
	thread_lib.o	\
	wait_control_block.o \
        mutex.o		\
	cond_var.o      \
	sem.o		\
	rwlocks.o

# Thread Group Library Support.
#
# Since libthrgrp.a depends on your thread library, the "buildable blank
# P3" we give you can't build libthrgrp.a.  Once you install your thread
# library and fix THREAD_OBJS above, uncomment this line to enable building
# libthrgrp.a:
410USER_LIBS_EARLY += libthrgrp.a

###########################################################################
# Object files for your syscall wrappers
###########################################################################
SYSCALL_OBJS = sc_lc_fork.o   \
	sc_lc_exec.o          \
	sc_lc_set_status.o    \
	sc_lc_vanish.o        \
	sc_lc_wait.o          \
	sc_lc_task_vanish.o   \
	sc_tm_gettid.o        \
	sc_tm_yield.o         \
	sc_tm_cas2i_runflag.o \
	sc_tm_get_ticks.o     \
	sc_tm_sleep.o         \
	sc_mm_new_pages.o     \
	sc_mm_remove_pages.o  \
	sc_con_getchar.o      \
	sc_con_readline.o     \
	sc_con_print.o        \
	sc_con_set_term_color.o \
	sc_con_set_cursor_pos.o \
	sc_con_get_cursor_pos.o \
	sc_misc_halt.o		\
	sc_misc_ls.o		\
	sc_spc_misbehave.o


###########################################################################
# Parts of your kernel
###########################################################################
#
# Kernel object files you want included from 410kern/
#
410KERNEL_OBJS = load_helper.o


#
# Kernel object files you provide in from kern/
#
KERNEL_OBJS = kernel.o 				\
	loader.o 				\
	malloc_wrappers.o 			\
	$(BOOT_DRVLIB_DIR)/bootdrivers.o	\
	$(BOOT_DRVLIB_DIR)/console_driver.o 	\
	$(BOOT_DRVLIB_DIR)/timer_driver.o       \
	$(BOOT_DRVLIB_DIR)/keyb_driver.o	\
        $(I386_UTIL_DIR)/i386systemregs.o	\
	$(I386_UTIL_DIR)/i386isrwrapper.o	\
	$(SYSCALL_DIR)/syscall.o		\
	$(SYSCALL_DIR)/syscallWrapper.o		\
	$(SYSCALL_DIR)/syscall_exec.o		\
	$(SYSCALL_DIR)/syscall_getticks.o	\
	$(SYSCALL_DIR)/syscall_gettid.o		\
	$(SYSCALL_DIR)/syscall_vanish.o		\
	$(SYSCALL_DIR)/syscall_taskvanish.o		\
	$(SYSCALL_DIR)/syscall_set_status.o	\
	$(SYSCALL_DIR)/syscall_fork.o		\
	$(SYSCALL_DIR)/syscall_console.o	\
	$(SYSCALL_DIR)/syscall_sleep.o		\
	$(SYSCALL_DIR)/syscall_wait.o		\
	$(SYSCALL_DIR)/syscall_yield.o		\
	$(SYSCALL_DIR)/syscall_threadfork.o	\
	$(SYSCALL_DIR)/syscall_ls.o		\
	$(SYSCALL_DIR)/syscall_halt.o		\
	$(SYSCALL_DIR)/syscall_pages.o		\
	$(SYSCALL_DIR)/syscall_cas2irunflag.o	\
	$(SYSCALL_DIR)/syscall_paramcheck.o	\
	$(FAULT_DIR)/faulthandlers.o            \
	$(VMM_DIR)/vmm.o			\
	$(TASK_DIR)/task.o			\
	$(TASK_DIR)/initTaskCode.o		\
	$(SCHED_DIR)/sched.o			\
	$(SCHED_DIR)/sync.o
