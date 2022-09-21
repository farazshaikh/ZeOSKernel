/** @file     kernel.c
 *  @brief    This file initializes the different components of the kernel
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */
#include <malloc.h>

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* memory includes. */
#include <lmm.h>                    /* lmm_remove_free() */

/* x86 specific includes */
#include <x86/seg.h>                /* install_user_segs() */
#include <x86/interrupt_defines.h>  /* interrupt_setup() */
#include <x86/asm.h>                /* enable_interrupts() */

/* Kernel includes */
#include <syscall_entry.h>
#include <vmm.h>

/*
 * state for kernel memory allocation.
 */
extern lmm_t malloc_lmm;

/*
 * Info about system gathered by the boot loader
 */
extern struct multiboot_info boot_info;

/** @brief Kernel entrypoint.
 *  
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
  KERN_RET_CODE ret;
    /*
     * Tell the kernel memory allocator which memory it can't use.
     * It already knows not to touch kernel image.
     */

    /* Everything above 16M */
    lmm_remove_free( &malloc_lmm, (void*)USER_MEM_START, -8 - USER_MEM_START );
    
    /* Everything below 1M  */
    lmm_remove_free( &malloc_lmm, (void*)0, 0x100000 );


    // you need not have any sim breaks anywhere use remote-gdb
    //SIM_break(); 


    /*
     * initialize the PIC so that IRQs and
     * exception handlers don't overlap in the IDT.
     */
    interrupt_setup();



    malloc_init();
    /* Boot Drivers Init */
    ret = faulthandler_init();
    if( KERN_SUCCESS != ret ) { 
      DUMP("faulthandler_init() failed with ret=%d",ret);
      panic("faulthandler_init() failed");
    }

    /* sleeping init */
    ret = sleep_init();
    if( KERN_SUCCESS != ret ) { 
      DUMP("sleep_init() failed with ret=%d",ret);
      panic("sleep_init() failed");
    }    

    /* Boot Drivers Init */
    ret = boot_driver_init();
    if( KERN_SUCCESS != ret ) { 
      DUMP("boot_driver_init() failed with ret=%d",ret);
      panic("boot_driver_init() failed");
    }

    
    /* Virtual  Memory Init */
    ret = vmm_init();
    if( KERN_SUCCESS != ret ) { 
      DUMP("vmm_int() failed with ret=%d",ret);
      panic("vmm_init failed");
    }

    /* system call init */
    ret = syscall_init();
    if( KERN_SUCCESS != ret ) { 
      DUMP("syscall_int() failed with ret=%d",ret);
      panic("syscall_init failed");
    }



    /* scheduler init */
    //enable_interrupts();   
    ret = sched_init();
    if( KERN_SUCCESS != ret ) { 
      DUMP("sched_init() failed with ret=%d",ret);
      panic("sched_init() failed");
    }

    //- Ordering requirement 
    //- Although scheduling has been initialized. NO ONE calls
    //- schedule() unless task_init completes which sets up the 
    //- idle thread.
    
    lprintf( "Hello from a brand new kernel!" );

    while (1) {
        continue;
    }

    return 0;
}
