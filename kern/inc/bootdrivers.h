/** @file     bootdrivers.h
 *  @brief    This file exports the boot driver initialization function
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#ifndef _BOOT_DRV_H
#define _BOOT_DRV_H

#include <kern_common.h>

KERN_RET_CODE boot_driver_init(void);

#endif // _BOOT_DRV_H
