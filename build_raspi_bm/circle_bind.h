/*
 *  circle_bind.h
 *  Daruma Basic
 *
 *  Interface for binding to circle kernel
 *
 *  Created by Toshi Nagata on 2017/03/31.
 *  Copyright 2017 Toshi Nagata.  All rights reserved.
*/

#ifndef __CIRCLE_BIND_H__
#define __CIRCLE_BIND_H__

#include <stdint.h>

#define CIRCLE_DEPTH 16  /*  Should match with DEPTH in circle/screen.h  */

typedef void MyKernelTimerHandler(unsigned int hTimer, void *pParam, void *pContext);

#ifdef __cplusplus
extern "C" {
#endif

	void bs_raspibm_init_framebuffer(void);
	void *bs_raspibm_get_framebuffer(void);
	int mount(const char *source);
	int umount(const char *target);
	void bs_blink(int n);
	void log_printf (const char *ptr, ...);
	uint64_t arm_systimer(void);

	unsigned int bs_start_kernel_timer(unsigned int nDelay, MyKernelTimerHandler *pHandler, void *pParam, void *pContext);
	void bs_cancel_kernel_timer(unsigned int hTimer);
	int bs_getkey(void);

#ifdef __cplusplus
}
#endif

#endif  /*  __CIRCLE_BIND_H__  */
