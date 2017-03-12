/*
 *  execcode.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 2016/01/23.
 *  Copyright 2016 Toshi Nagata.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <signal.h>    /*  sigaction()  */
#include <sys/time.h>  /*  setitimer()  */
#include <unistd.h>    /*  usleep()     */

#include "daruma.h"
#include "gencode.h"
#include "screen.h"
#include "transmessage.h"
#include "y.tab.h"

/*  Runtime VM registers  */
static Off_t sVMCodePos;          /*  Program counter (offset from gVMCodeBasePtr)  */
static u_int8_t *sVMStackPtr;     /*  Stack pointer  */
static u_int8_t *sVMFramePtr;     /*  Frame pointer  */
static u_int8_t *sVMArgPtr;       /*  Argument pointer  */

#define sVMStackIntPtr ((Int *)sVMStackPtr)
#define sVMStackFloatPtr ((Float *)sVMStackPtr)
#define sVMStackOffPtr ((Off_t *)sVMStackPtr)

static u_int8_t sTraceFlag;

static u_int8_t sRunning;  /*  Non-zero if code is running  */

/*  Read statements  */
static Off_t sVMDataPos;  /*  Position of the C_DATA or C_CDATA instruction  */
static DataAccessor sDataAccessor;

/*  Accessing arguments from built-in functions  */
static DataAccessor sBuiltInArgAccessor;
static const u_int8_t *sBuiltInArgSig;

/*  Runtime record of program counter  */
#define gPCRecordSize 32
Off_t gPCRecord[gPCRecordSize];
int gPCRecordTop;

#if 0
#pragma mark ====== Timer Interrupt ======
#endif

volatile u_int8_t gTimerInvoked;
volatile int32_t gTimerCount;

#if defined(__BAREMETAL__)

#include <uspios.h>

volatile unsigned s_timer_id = 0;

static void
s_timer_action(unsigned hTimer, void *pParam, void *pContext)
{
	gTimerCount++;
	gTimerInvoked = 1;
/*	s_timer_id = StartKernelTimer(1, s_timer_action, NULL, NULL); */
}


static void
s_stop_interval_timer(void)
{
	if (s_timer_id != 0) {
		CancelKernelTimer(s_timer_id);
		s_timer_id = 0;
	}
}

static void
s_start_interval_timer(void)
{
	if (s_timer_id == 0) {
		s_timer_id = StartKernelTimer(-1, s_timer_action, NULL, NULL);
	}
}

#else

static void
s_signal_action(int n)
{
	gTimerCount++;
	gTimerInvoked = 1;
}

static void
s_stop_interval_timer(void)
{
	struct itimerval val;
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 0;
	val.it_interval = val.it_value;
	setitimer(ITIMER_REAL, &val, NULL);
}

static void
s_start_interval_timer(void)
{
	struct itimerval val;
	struct sigaction act;
	gTimerCount = 0;
	act.sa_handler = s_signal_action;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART; /*  Don't interrupt system calls on SIGALRM  */
	sigaction(SIGALRM, &act, NULL);
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 1000;
	val.it_interval = val.it_value;
	setitimer(ITIMER_REAL, &val, NULL);
}

#endif /* __BAREMETAL__ */

static struct timeval s_start_timeval;

u_int64_t
bs_uptime(int init)
{
	struct timeval tval;
	gettimeofday(&tval, NULL);
	if (init)
		s_start_timeval = tval;
	return ((u_int64_t)(tval.tv_sec - s_start_timeval.tv_sec)) * 1000000 + tval.tv_usec - s_start_timeval.tv_usec;
}

#if 0
#pragma mark ====== Arg accessor ======
#endif

void
bs_reset_arg_accessor(void)
{
	INIT_DATA(sBuiltInArgAccessor);
}

int
bs_get_next_int_arg(Int *ip)
{
	if (NEXT_DATA(sBuiltInArgSig, sBuiltInArgAccessor)) {
		if (sBuiltInArgAccessor.type == BS_TYPE_INTEGER)
			*ip = *((Int *)(sVMFramePtr - (sizeof(Off_t) * 3 + sBuiltInArgAccessor.ofs)));
		else if (sBuiltInArgAccessor.type == BS_TYPE_FLOAT)
			*ip = (Int)*((Float *)(sVMFramePtr - (sizeof(Off_t) * 3 + sBuiltInArgAccessor.ofs)));
		else return 0;
		return BS_TYPE_INTEGER;
	} else return 0;	
}

int
bs_get_next_float_arg(Float *fp)
{
	if (NEXT_DATA(sBuiltInArgSig, sBuiltInArgAccessor)) {
		if (sBuiltInArgAccessor.type == BS_TYPE_INTEGER)
			*fp = (Float)*((Int *)(sVMFramePtr - (sizeof(Off_t) * 3 + sBuiltInArgAccessor.ofs)));
		else if (sBuiltInArgAccessor.type == BS_TYPE_FLOAT)
			*fp = *((Float *)(sVMFramePtr - (sizeof(Off_t) * 3 + sBuiltInArgAccessor.ofs)));
		else return 0;
		return BS_TYPE_FLOAT;
	} else return 0;	
}

int
bs_get_next_str_arg(Off_t *sp)
{
	if (NEXT_DATA(sBuiltInArgSig, sBuiltInArgAccessor)) {
		if (sBuiltInArgAccessor.type == BS_TYPE_STRING)
			*sp = *((Off_t *)(sVMFramePtr - (sizeof(Off_t) * 3 + sBuiltInArgAccessor.ofs)));
		else return 0;
		return BS_TYPE_STRING;
	} else return 0;	
}

int
bs_get_next_arg_type_and_ptr(void **outptr)
{
	if (NEXT_DATA(sBuiltInArgSig, sBuiltInArgAccessor)) {
		*outptr = sVMFramePtr - (sizeof(Off_t) * 3 + sBuiltInArgAccessor.ofs);
		return sBuiltInArgAccessor.type;
	} else {
		return 0;
	}
}

#if 0
#pragma mark ====== Stack handler ======
#endif

void
bs_push_int_value(Int ival)
{
	sVMStackIntPtr[0] = ival;
	sVMStackPtr += sizeof(Int);
}

void
bs_push_float_value(Float dval)
{
	sVMStackFloatPtr[0] = dval;
	sVMStackPtr += sizeof(Float);
}

void
bs_push_str_value(Off_t sval)
{
	sVMStackOffPtr[0] = sval;
	sVMStackPtr += sizeof(Off_t);
}

int
bs_stackcheck(void)
{
	if (sVMStackPtr < gVMStackBasePtr) {
		bs_runtime_error(MSG_(BS_M_STACK_UNDERFLOW));
		return 1;
	} else if (sVMStackPtr >= gVMStackEndPtr - 10 * sizeof(Off_t)) {
		bs_runtime_error(MSG_(BS_M_STACK_OVERFLOW));
		return 1;
	}
	return 0;
}

/*  Move the runtime stack pointer (called from bs_realloc_block()  */
void
bs_relocate_runtime_stack(Off_t dsize)
{
	if (sVMStackPtr != NULL) {
		sVMStackPtr += dsize;
		sVMFramePtr += dsize;
	}
	if (sVMArgPtr != NULL)
	sVMArgPtr += dsize;
}

#if 0
#pragma mark ====== External Interface ======
#endif

int
bs_exec_set_trace_flag(int flag)
{
	int old_flag = sTraceFlag;
	sTraceFlag = (flag != 0);
	return old_flag;
}

Off_t
bs_codepos(void)
{
	return sVMCodePos;
}

int
bs_is_running(void)
{
	return sRunning;
}

/*  Prepare for execution of VM code  */
/*  If new_run is non-zero, then runtime memory is all cleared, and
    stack/frame/argument pointers are also initialized  */
int
bs_execinit(Off_t progEntry, int new_run)
{
	sVMCodePos = progEntry;
	if (new_run) {
		bs_init_runtime_memory();
		sVMFramePtr = gVMStackBasePtr;
		sVMStackPtr = sVMFramePtr + ((FuncInfo *)gFuncInfoBasePtr)[0].lsize;
		sVMArgPtr = NULL;
		sVMDataPos = 0;
		gVarTop = gParserInfo.varreq;
		gPatInfoTop = gPatternTop = 0;
		INIT_DATA(sDataAccessor);
	}
	return 0;
}

#define RECORD_PC(_n)	(gPCRecord[gPCRecordTop++ % gPCRecordSize] = sVMCodePos + _n)

int
bs_execcode(void)
{
	u_int8_t code;
	int retval = 0;
	int32_t counter = 0;
	
	sRunning = 1;
	s_start_interval_timer();
	
loop:
#if 1  /*  For debug  */
	if (sTraceFlag) {
		debug_printf("SP: %4d  ", sVMStackPtr - gVMStackBasePtr);
		bs_dump_vmcode(sVMCodePos, 1);
	}
#endif

#if 1
	if (gTimerInvoked) {
	/*	printf("%06x\n", sVMCodePos); */
		static u_int64_t tm0 = 0, tm;
		gTimerInvoked = 0;
		counter = 0;
		if (tm0 == 0)
			tm0 = bs_uptime(0);
		tm = bs_uptime(0);
		if (tm - tm0 > 100000) {
			/*  Every 0.1 seconds (sparse enough to avoid retarding execution, and
			 frequently enough to respond to user request within reasonable time)  */
			/*  Also, the screen update is performed at this time.  */
			bs_update_screen();
			tm0 = tm;
			if (bs_getch_with_timeout(0) == 3) {
				bs_runtime_error(MSG_(BS_M_USER_INTERRUPT));
				retval = 2;
				goto exit;
			}
		}
	}
#endif
	
	switch (code = gVMCodeBasePtr[sVMCodePos++]) {
		u_int8_t *up;
		Off_t *op;
		Int ival, i2, i3;
		Off_t sval;
		Float dval;
		char buf[64];

		case C_NOP:	break;

#pragma mark ------ Number operations ------			
		case C_PUSH_INT:
			sVMCodePos = bs_load_operand(sVMCodePos, sVMStackIntPtr);
			sVMStackPtr += sizeof(Int);
			break;
		case C_PUSH_FLT:
			sVMStackFloatPtr[0] = bs_load_float(sVMCodePos);
			sVMCodePos += sizeof(Float);
			sVMStackPtr += sizeof(Float);
			break;
		case C_PUSH_STRL:
		case C_PUSH_DIMREF:
			sVMCodePos = bs_load_string(sVMCodePos, &sval);
			sVMStackOffPtr[0] = sval | kMSBOff;   /*  String literal or DIM reference  */
			sVMStackPtr += sizeof(Off_t);
			break;			
		case C_PUSH_REF:
		case C_PUSH_FREF:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			sVMStackOffPtr[0] = ival;
			sVMStackPtr += sizeof(Off_t);
			break;
			
#pragma mark ------ Binary operations ------
		case C_ADD_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] += sVMStackIntPtr[0];
			break;
		case C_SUB_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] -= sVMStackIntPtr[0];
			break;
		case C_MUL_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] *= sVMStackIntPtr[0];
			break;
		case C_DIV_INT:
			ival = sVMStackIntPtr[-1];
			if (ival == 0) {
				sVMStackPtr -= sizeof(Int) * 2;
				bs_runtime_error(MSG_(BS_M_ZERO_DIVISION));
				retval = 1;
				goto exit;
			}
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] /= ival;
			break;
		case C_MOD_INT:
			ival = sVMStackIntPtr[-1];
			if (ival == 0) {
				sVMStackPtr -= sizeof(Int) * 2;
				bs_runtime_error(MSG_(BS_M_ZERO_DIVISION));
				retval = 1;
				goto exit;
			}
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] %= ival;
			break;
		case C_LT_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] < sVMStackIntPtr[0]);
			break;
		case C_GT_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] > sVMStackIntPtr[0]);
			break;
		case C_LTEQ_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] <= sVMStackIntPtr[0]);
			break;
		case C_GTEQ_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] >= sVMStackIntPtr[0]);
			break;
		case C_EQ_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] == sVMStackIntPtr[0]);
			break;
		case C_NEQ_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] != sVMStackIntPtr[0]);
			break;
		case C_ADD_FLT:
			sVMStackPtr -= sizeof(Float);
			sVMStackFloatPtr[-1] += sVMStackFloatPtr[0];
			break;
		case C_SUB_FLT:
			sVMStackPtr -= sizeof(Float);
			sVMStackFloatPtr[-1] -= sVMStackFloatPtr[0];
			break;
		case C_MUL_FLT:
			sVMStackPtr -= sizeof(Float);
			sVMStackFloatPtr[-1] *= sVMStackFloatPtr[0];
			break;
		case C_DIV_FLT:
			sVMStackPtr -= sizeof(Float);
			sVMStackFloatPtr[-1] /= sVMStackFloatPtr[0];
			break;
		case C_LT_FLT:
			ival = (sVMStackFloatPtr[-2] < sVMStackFloatPtr[-1]);
			sVMStackPtr -= sizeof(Float) * 2 - sizeof(Int);
			sVMStackIntPtr[-1] = ival;
			break;
		case C_GT_FLT:
			ival = (sVMStackFloatPtr[-2] > sVMStackFloatPtr[-1]);
			sVMStackPtr -= sizeof(Float) * 2 - sizeof(Int);
			sVMStackIntPtr[-1] = ival;
			break;
		case C_LTEQ_FLT:
			ival = (sVMStackFloatPtr[-2] <= sVMStackFloatPtr[-1]);
			sVMStackPtr -= sizeof(Float) * 2 - sizeof(Int);
			sVMStackIntPtr[-1] = ival;
			break;
		case C_GTEQ_FLT:
			ival = (sVMStackFloatPtr[-2] >= sVMStackFloatPtr[-1]);
			sVMStackPtr -= sizeof(Float) * 2 - sizeof(Int);
			sVMStackIntPtr[-1] = ival;
			break;
		case C_EQ_FLT:
			ival = (sVMStackFloatPtr[-2] == sVMStackFloatPtr[-1]);
			sVMStackPtr -= sizeof(Float) * 2 - sizeof(Int);
			sVMStackIntPtr[-1] = ival;
			break;
		case C_NEQ_FLT:
			ival = (sVMStackFloatPtr[-2] != sVMStackFloatPtr[-1]);
			sVMStackPtr -= sizeof(Float) * 2 - sizeof(Int);
			sVMStackIntPtr[-1] = ival;
			break;
		case C_ADD_STR: {
			const char *p1 = bs_get_string_ptr(sVMStackOffPtr[-2]);
			const char *p2 = bs_get_string_ptr(sVMStackOffPtr[-1]);
			char *p;
			i2 = strlen(p1);
			i3 = strlen(p2);
			sval = bs_new_runtime_string(NULL, i2 + i3 + 1);
			if (sval == kInvalidOff) {
				bs_runtime_error(MSG_(BS_M_OUT_OF_MEMORY_STRING));
				retval = 1;
				goto exit;
			}
			p = bs_get_string_ptr(sval);
			strcpy(p, p1);
			strcpy(p + i2, p2);
			bs_release_string(sVMStackOffPtr[-2]);
			bs_release_string(sVMStackOffPtr[-1]);
			sVMStackPtr -= sizeof(Off_t);
			sVMStackOffPtr[-1] = sval;
			break;
		}
		case C_LT_STR:
		case C_GT_STR:
		case C_LTEQ_STR:
		case C_GTEQ_STR:
		case C_EQ_STR:
		case C_NEQ_STR: {
			const char *p1 = bs_get_string_ptr(sVMStackOffPtr[-2]);
			const char *p2 = bs_get_string_ptr(sVMStackOffPtr[-1]);
			i2 = strcmp(p1, p2);
			bs_release_string(sVMStackOffPtr[-2]);
			bs_release_string(sVMStackOffPtr[-1]);
			switch (code) {
				case C_LT_STR: i3 = (i2 < 0); break;
				case C_GT_STR: i3 = (i2 > 0); break;
				case C_LTEQ_STR: i3 = (i2 <= 0); break;
				case C_GTEQ_STR: i3 = (i2 >= 0); break;
				case C_EQ_STR: i3 = (i2 == 0); break;
				case C_NEQ_STR: i3 = (i2 != 0); break;
			}
			sVMStackPtr -= sizeof(Off_t) * 2 - sizeof(Int);
			sVMStackIntPtr[-1] = i3;
			break;
		}
		case C_AND_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] & sVMStackIntPtr[0]);
			break;
		case C_OR_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] | sVMStackIntPtr[0]);
			break;			
		case C_XOR_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] ^ sVMStackIntPtr[0]);
			break;			
		case C_LSHIFT_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] << sVMStackIntPtr[0]);
			break;			
		case C_RSHIFT_INT:
			sVMStackPtr -= sizeof(Int);
			sVMStackIntPtr[-1] = (sVMStackIntPtr[-1] >> sVMStackIntPtr[0]);
			break;
			
#pragma mark ------ Unary operations ------
		case C_NEG_INT:
			sVMStackIntPtr[-1] = -sVMStackIntPtr[-1];
			break;			
		case C_NEG_FLT:
			sVMStackFloatPtr[-1] = -sVMStackFloatPtr[-1];
			break;			
		case C_NOT_INT:
			sVMStackIntPtr[-1] = !sVMStackIntPtr[-1];
			break;
		case C_FLT_TO_INT:
			ival = floor(sVMStackFloatPtr[-1] + 0.5);
			sVMStackPtr -= sizeof(Float) - sizeof(Int);
			sVMStackIntPtr[-1] = ival;
			break;
		case C_INT_TO_FLT:
			dval = (Float)sVMStackIntPtr[-1];
			sVMStackPtr += sizeof(Float) - sizeof(Int);
			sVMStackFloatPtr[-1] = dval;
			break;

#pragma mark ------ Indirect addressing ------
	/*  Indirect addressing: pop an address from the stack and push the content of the address */
		case C_LOAD_INT:
			up = gVarBasePtr + sVMStackOffPtr[-1];
			sVMStackPtr += sizeof(Int) - sizeof(Off_t);
			sVMStackIntPtr[-1] = ((Int *)up)[0];
			break;
		case C_LOAD_BYTE:
			up = gVarBasePtr + sVMStackOffPtr[-1];
			sVMStackPtr += sizeof(Int) - sizeof(Off_t);
			sVMStackIntPtr[-1] = up[0];
			break;
		case C_LOAD_FLT:
			up = gVarBasePtr + sVMStackOffPtr[-1];
			sVMStackPtr += sizeof(Float) - sizeof(Off_t);
			sVMStackFloatPtr[-1] = ((Float *)up)[0];
			break;
		case C_LOAD_STR:
		case C_LOAD_REF:
			up = gVarBasePtr + sVMStackOffPtr[-1];
			sVMStackOffPtr[-1] = ((Off_t *)up)[0];
			if (code == C_LOAD_STR)
				bs_retain_string(sVMStackOffPtr[-1]);
			break;
			
	/*  Indirect addressing: pop an address and a value from the stack and store the number
	 to the address  */
		case C_STORE_INT:
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int) + sizeof(Off_t);
			up = gVarBasePtr + sVMStackOffPtr[0];
			((Int *)up)[0] = ival;
			break;
		case C_STORE_BYTE:
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int) + sizeof(Off_t);
			up = gVarBasePtr + sVMStackOffPtr[0];
			up[0] = ival;
			break;
		case C_STORE_FLT:
			dval = sVMStackFloatPtr[-1];
			sVMStackPtr -= sizeof(Float) + sizeof(Off_t);
			up = gVarBasePtr + sVMStackOffPtr[0];
			((Float *)up)[0] = dval;
			break;
		case C_STORE_STR:
		case C_STORE_REF:
			sVMStackPtr -= 2 * sizeof(Off_t);
			up = gVarBasePtr + sVMStackOffPtr[0];
			if (code == C_STORE_STR)
				bs_release_string(((Off_t *)up)[0]);
			((Off_t *)up)[0] = sVMStackOffPtr[1];
			break;

	/*  Indirect addressing: offset from the frame pointer  */
		case C_FLOAD_INT:
			up = sVMFramePtr + sVMStackOffPtr[-1];
			sVMStackPtr += sizeof(Int) - sizeof(Off_t);
			sVMStackIntPtr[-1] = ((Int *)up)[0];
			break;
		case C_FLOAD_BYTE:
			up = sVMFramePtr + sVMStackOffPtr[-1];
			sVMStackPtr += sizeof(Int) - sizeof(Off_t);
			sVMStackIntPtr[-1] = up[0];
			break;
		case C_FLOAD_FLT:
			up = sVMFramePtr + sVMStackOffPtr[-1];
			sVMStackPtr += sizeof(Float) - sizeof(Off_t);
			sVMStackFloatPtr[-1] = ((Float *)up)[0];
			break;
		case C_FLOAD_STR:
		case C_FLOAD_REF:
			up = sVMFramePtr + sVMStackOffPtr[-1];
			sVMStackOffPtr[-1] = ((Off_t *)up)[0];
			if (code == C_LOAD_STR)
				bs_retain_string(sVMStackOffPtr[-1]);
			break;
			
			/*  Indirect addressing: pop an address and a value from the stack and store the number
			 to the address  */
		case C_FSTORE_INT:
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int) + sizeof(Off_t);
			up = sVMFramePtr + sVMStackOffPtr[0];
			((Int *)up)[0] = ival;
			break;
		case C_FSTORE_BYTE:
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int) + sizeof(Off_t);
			up = sVMFramePtr + sVMStackOffPtr[0];
			up[0] = ival;
			break;
		case C_FSTORE_FLT:
			dval = sVMStackFloatPtr[-1];
			sVMStackPtr -= sizeof(Float) + sizeof(Off_t);
			up = sVMFramePtr + sVMStackOffPtr[0];
			((Float *)up)[0] = dval;
			break;
		case C_FSTORE_STR:
		case C_FSTORE_REF:
			sVMStackPtr -= 2 * sizeof(Off_t);
			up = sVMFramePtr + sVMStackOffPtr[0];
			if (code == C_STORE_STR)
				bs_release_string(((Off_t *)up)[0]);
			((Off_t *)up)[0] = sVMStackOffPtr[1];
			break;
		
	/*  Special stack operations  */
		case C_SMOVE_INT:
			sVMCodePos = bs_load_operand(sVMCodePos, &i2);
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int);
			*((Int *)(sVMStackPtr + i2)) = ival;
			break;
		case C_SMOVE_FLT:
			sVMCodePos = bs_load_operand(sVMCodePos, &i2);
			dval = sVMStackFloatPtr[-1];
			sVMStackPtr -= sizeof(Float);
			*((Float *)(sVMStackPtr + i2)) = dval;
			break;
		case C_SMOVE_STR:
			/*  Don't release the old value (assumes garbage)  */
			sVMCodePos = bs_load_operand(sVMCodePos, &i2);
			sval = sVMStackOffPtr[-1];
			sVMStackPtr -= sizeof(Off_t);
			*((Off_t *)(sVMStackPtr + i2)) = sval;
			break;

		case C_RELSTR_SP:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			bs_release_string(*((Off_t *)(sVMStackPtr + ival)));
			break;
			
#pragma mark ------ Stack operations ------
	/*  Stack operations  */
		case C_DUP_REF:
			sVMStackOffPtr[0] = sVMStackOffPtr[-1];
			sVMStackPtr += sizeof(Off_t);
			break;
		
		case C_INC_SP:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			sVMStackPtr += ival;
			break;

		case C_EXCHANGE:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			i2 = gTypeSize[ival & 7];
			i3 = gTypeSize[(ival >> 3) & 7];
			memmove(buf, sVMStackPtr - (i2 + i3), i2 + i3);
			memmove(sVMStackPtr - (i2 + i3), buf + i3, i2);
			memmove(sVMStackPtr - i3, buf, i3);
			break;
			
#if 0
		case C_POP:
			gStackPtr--;
			break;
		case C_POPN:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			gStackPtr -= ival;
			break;
		case C_PUSH:
			*gStackPtr++ = gZeroTokenValue;
			break;
		case C_PUSHN:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			while (--ival >= 0) {
				*gStackPtr++ = gZeroTokenValue;
			}
			break;
#endif /* 0 */
			
#pragma mark ------ Frame pointer operations ------
		case C_LINK:
			sVMStackOffPtr[0] = sVMFramePtr - gVMStackBasePtr;
			sVMFramePtr = sVMStackPtr + sizeof(Off_t);
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			sVMStackPtr = sVMFramePtr + ival;
			break;
		case C_RETURN:
			RECORD_PC(-1);
			sVMStackPtr = sVMFramePtr - sizeof(Off_t) * 2;
			sVMFramePtr = gVMStackBasePtr + sVMStackOffPtr[1];
			sVMCodePos = sVMStackOffPtr[0];
			RECORD_PC(0);
			break;
		case C_RET_INT:
			RECORD_PC(-1);
			up = sVMStackPtr - sizeof(Int);
			sVMStackPtr = sVMFramePtr - sizeof(Off_t) * 2;
			sVMFramePtr = gVMStackBasePtr + sVMStackOffPtr[1];
			sVMCodePos = sVMStackOffPtr[0];
			sVMStackIntPtr[0] = *((Int *)up);
			sVMStackPtr += sizeof(Int);
			RECORD_PC(0);
			break;
		case C_RET_FLT:
			RECORD_PC(-1);
			up = sVMStackPtr - sizeof(Float);
			sVMStackPtr = sVMFramePtr - sizeof(Off_t) * 2;
			sVMFramePtr = gVMStackBasePtr + sVMStackOffPtr[1];
			sVMCodePos = sVMStackOffPtr[0];
			sVMStackFloatPtr[0] = *((Float *)up);
			sVMStackPtr += sizeof(Float);
			RECORD_PC(0);
			break;
		case C_RET_STR:
			RECORD_PC(-1);
			up = sVMStackPtr - sizeof(Off_t);
			sVMStackPtr = sVMFramePtr - sizeof(Off_t) * 2;
			sVMFramePtr = gVMStackBasePtr + sVMStackOffPtr[1];
			sVMCodePos = sVMStackOffPtr[0];
			sVMStackOffPtr[0] = *((Off_t *)up);
			sVMStackPtr += sizeof(Off_t);
			RECORD_PC(0);
			break;
			
#pragma mark ------ Branch instructions ------
		case C_BRA:
			RECORD_PC(-1);
			sVMCodePos = bs_load_progpos(sVMCodePos, &ival);
			if (ival < 0 || ival >= gVMCodeTop)
				goto pc_error;
			sVMCodePos = ival;
			RECORD_PC(0);
			break;
		case C_BZ:
			RECORD_PC(-1);
			sVMCodePos = bs_load_progpos(sVMCodePos, &ival);
			if (ival < 0 || ival >= gVMCodeTop)
				goto pc_error;
			sVMStackPtr -= sizeof(Int);
			if (sVMStackIntPtr[0] == 0)
				sVMCodePos = ival;
			RECORD_PC(0);
			break;
		case C_BNZ:
			RECORD_PC(-1);
			sVMCodePos = bs_load_progpos(sVMCodePos, &ival);
			if (ival < 0 || ival >= gVMCodeTop)
				goto pc_error;
			sVMStackPtr -= sizeof(Int);
			if (sVMStackIntPtr[0] != 0)
				sVMCodePos = ival;
			RECORD_PC(0);
			break;
		case C_BSR:
			RECORD_PC(-1);
			sVMCodePos = bs_load_progpos(sVMCodePos, &ival);
			if (ival < 0 || ival >= gVMCodeTop)
				goto pc_error;
			sVMStackOffPtr[0] = sVMCodePos;
			sVMStackPtr += sizeof(Off_t);
			sVMCodePos = ival;
			RECORD_PC(0);
			break;
			
		pc_error: bs_runtime_error(MSG_(BS_M_JUMP_ADDRESS_OUT_OF_RANGE));
			retval = 1;
			goto exit;

#pragma mark ------ Special compare instructions ------
			
		case C_NEXT_INT:
		case C_NEXT_INTL:
		case C_FORTEST_INT:
		case C_FORTEST_INTL: {
			Int *ip1, *ip2;
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			if (code == C_FORTEST_INT || code == C_NEXT_INT)
				ip1 = (Int *)(gVarBasePtr + *((Off_t *)(sVMFramePtr + ival)));
			else
				ip1 = (Int *)(sVMFramePtr + *((Off_t *)(sVMFramePtr + ival)));
			ip2 = (Int *)(sVMFramePtr + ival + sizeof(Off_t) + sizeof(Int));
			i3 = *ip2;
			if (code == C_NEXT_INT || code == C_NEXT_INTL)
				*ip1 += i3;  /*  Increment the control variable  */
			i2 = *ip1 - *((Int *)(sVMFramePtr + ival + sizeof(Off_t)));
			if ((i2 > 0 && i3 > 0) || (i2 < 0 && i3 < 0))
				sVMStackIntPtr[0] = 1;
			else sVMStackIntPtr[0] = 0;
			sVMStackPtr += sizeof(Int);
			break;
		}
		case C_NEXT_FLT:
		case C_NEXT_FLTL:
		case C_FORTEST_FLT:
		case C_FORTEST_FLTL: {
			Float *fp1, *fp2, f2, f3;
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			if (code == C_FORTEST_FLT || code == C_NEXT_FLT)
				fp1 = (Float *)(gVarBasePtr + *((Off_t *)(sVMFramePtr + ival)));
			else
				fp1 = (Float *)(sVMFramePtr + *((Off_t *)(sVMFramePtr + ival)));
			fp2 = (Float *)(sVMFramePtr + ival + sizeof(Off_t) + sizeof(Float));
			f3 = *fp2;
			if (code == C_NEXT_FLT || code == C_NEXT_FLTL)
				*fp1 += f3;  /*  Increment the control variable  */
			f2 = *fp1 - *((Float *)(sVMFramePtr + ival + sizeof(Off_t)));
			if ((f2 > 0.0 && f3 > 0.0) || (f2 < 0.0 && f3 < 0.0))
				sVMStackIntPtr[0] = 1;
			else sVMStackIntPtr[0] = 0;
			sVMStackPtr += sizeof(Int);
			break;
		}
			
#pragma mark ------ I/O instructions ------
		case C_NEWLINE:
			bs_puts("\n");
			break;
		case C_PUTC:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			buf[0] = ival;
			buf[1] = 0;
			bs_puts(buf);
			break;
		case C_PRINT_INT:
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int);
			snprintf(buf, sizeof buf, "%d", ival);
			bs_puts(buf);
			break;
		case C_PRINT_FLT:
			dval = sVMStackFloatPtr[-1];
			sVMStackPtr -= sizeof(Float);
			snprintf(buf, sizeof buf, "%#.16g", dval);
			bs_puts(buf);
			break;
		case C_PRINT_STR:
			sval = sVMStackOffPtr[-1];
			sVMStackPtr -= sizeof(Off_t);
			bs_puts(bs_get_string_ptr(sval));
			bs_release_string(sval);
			break;

#pragma mark ------ DIM-related instructions ------

			/*  DIM-related instructions  */
			/*  DEF_DIM_XXX: pop the dimension size and the number of elements in 
			 each dimension from the stack, allocate runtime memory, and
			 keep the size information in the header of the allocated memory.  */
			/*  Stack[-1]: dimension=N, Stack[-2]: size N-1, Stack[-3]: size N-2, ..., Stack[-N-1]: size 0 */
			/*  One operand: address of the var space, which keeps the top offset of the
			 allocated memory  */
			/*  Throws exception if any of the dimension size is non-positive or malloc() fails  */
		case C_DEF_DIM_INT:
			i3 = sizeof(Int);
			goto def_dim;
		case C_DEF_DIM_FLT:
			i3 = sizeof(Float);
			goto def_dim;
		case C_DEF_DIM_STR:
			i3 = sizeof(Off_t);
			goto def_dim;
		case C_DEF_DIM_BYTE:
			i3 = sizeof(u_int8_t);
		def_dim: {
			Int i4, i5;
			i2 = sVMStackIntPtr[-1];  /*  Dimension size  */
			sVMStackPtr -= sizeof(Int) * (1 + i2);
			i5 = 1;
			for (i4 = 0; i4 < i2; i4++) {
				i5 = i5 * sVMStackIntPtr[i4];
				if (i5 <= 0)
					break;
			}
			i5 *= i3;
			i3 = gVarTop;
			if (i5 <= 0) {
				/*  Runtime exception  */
				bs_runtime_error("Zero or negative size in DIM declaration");
				retval = 1;
				goto exit;
			} else {
				/*  Allocate runtime memory  */
				i4 = gVarTop + i5 + sizeof(Off_t) * (1 + i2);
				if (i4 >= gVarEnd - gVarBase) {
					/*  Try to expand var area (size in 4K unit) */
					i4 = ((i4 + 4095) / 4096) * 4096;
					if (bs_realloc_block(MEM_VAR, i4) < 0) {
						/*  Runtime exception: out of memory  */
						bs_runtime_error(MSG_(BS_M_OUT_OF_MEMORY_DIM_ALLOC));
						retval = 1;
						goto exit;
					}
				} else {
					op = (Off_t *)(gVarBasePtr + gVarTop);
					*op++ = i2;
					for (i4 = 0; i4 < i2; i4++) {
						*op++ = sVMStackIntPtr[i4];
					}
					memset(op, 0, i5);
					gVarTop += i5 + sizeof(Off_t) * (1 + i2);
				}
			}
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			((Off_t *)(gVarBasePtr + ival))[0] = i3;
			break;
		}
			
			/*  GET_DIM_XXX: calc the address of the dimension element and push to the stack.
			 The address is calculated from the header of the DIM memory chunk, whose
			 address is pointed by the operand  */
		case C_GET_DIM_INT:
			i3 = sizeof(Int);
			goto get_dim;
		case C_GET_DIM_FLT:
			i3 = sizeof(Float);
			goto get_dim;
		case C_GET_DIM_STR:
			i3 = sizeof(Off_t);
			goto get_dim;
		case C_GET_DIM_BYTE:
			i3 = sizeof(u_int8_t);
		get_dim: {
			Int i4, i5;
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			ival = ((Off_t *)(gVarBasePtr + ival))[0];
			op = (Off_t *)(gVarBasePtr + ival);
			i2 = *op++;
			if (sVMStackIntPtr[-1] != i2) {
				/*  Runtime exception: dimension does not match  */
				bs_runtime_error(MSG_(BS_M_DIM_NO_MATCH_RUNTIME));
				retval = 1;
				goto exit;
			}
			sVMStackPtr -= (i2 + 1) * sizeof(Int);
			i5 = 0;
			for (i4 = 0; i4 < i2; i4++) {
				i5 = i5 * (*op++) + sVMStackIntPtr[i4];
			}
			i5 = i5 * i3 + ival + sizeof(Off_t) * (i2 + 1);
			sVMStackOffPtr[0] = i5;
			sVMStackPtr += sizeof(Off_t);
			break;
		}

			/*  DIM initializer support  */
		case C_PREP_DIM:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			ival = ((Off_t *)(gVarBasePtr + ival))[0];
			op = (Off_t *)(gVarBasePtr + ival);
			i2 = *op++;
			i3 = 1;
			while (--i2 >= 0)
				i3 *= *op++;
			sVMStackOffPtr[0] = (Off_t)i3;
			sVMStackOffPtr[1] = (u_int8_t *)op - gVarBasePtr;
			sVMStackPtr += sizeof(Off_t) * 2;
			break;
		case C_STORELOOP_INT:
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int);
			if (--(sVMStackOffPtr[-2]) < 0)
				goto storeloop_error;
			*((Int *)(gVarBasePtr + sVMStackOffPtr[-1])) = ival;
			sVMStackOffPtr[-1] += sizeof(Int);
			break;
		case C_STORELOOP_FLT:
			dval = sVMStackFloatPtr[-1];
			sVMStackPtr -= sizeof(Float);
			if (--(sVMStackOffPtr[-2]) < 0)
				goto storeloop_error;
			*((Float *)(gVarBasePtr + sVMStackOffPtr[-1])) = dval;
			sVMStackOffPtr[-1] += sizeof(Float);
			break;
		case C_STORELOOP_STR:
			sval = sVMStackOffPtr[-1];
			sVMStackPtr -= sizeof(Off_t);
			if (--(sVMStackOffPtr[-2]) < 0)
				goto storeloop_error;
			*((Off_t *)(gVarBasePtr + sVMStackOffPtr[-1])) = sval;
			sVMStackOffPtr[-1] += sizeof(Off_t);
			break;
		case C_STORELOOP_BYTE:
			ival = sVMStackIntPtr[-1];
			sVMStackPtr -= sizeof(Int);
			if (--(sVMStackOffPtr[-2]) < 0)
				goto storeloop_error;
			*((u_int8_t *)(gVarBasePtr + sVMStackOffPtr[-1])) = ival;
			sVMStackOffPtr[-1]++;
			break;
		storeloop_error:
			bs_runtime_error(MSG_(BS_M_TOO_MANY_DIM_INIT));
			retval = 1;
			goto exit;
#pragma mark ------ Built-in functions ------
			
		case C_BUILTIN_FUNC:
		case C_BUILTIN_SUB:
			sVMCodePos = bs_load_operand(sVMCodePos, &ival);
			memset(&sBuiltInArgAccessor, 0, sizeof(sBuiltInArgAccessor));
			sBuiltInArgSig = (const u_int8_t *)bs_get_string_ptr(sVMStackOffPtr[-1]);
			sVMStackOffPtr[0] = 0;  /*  Return address (dummy)  */
			sVMStackOffPtr[1] = sVMFramePtr - gVMStackBasePtr;
			sVMFramePtr = (sVMStackPtr += sizeof(Off_t) * 2);
			i3 = (*gBuiltIns[ival].entry)();
			if (i3 == 0) {
				up = sVMStackPtr;
				sVMStackPtr -= sizeof(Off_t) * 2;
				sVMFramePtr = gVMStackBasePtr + ((Off_t *)sVMFramePtr)[-1];
				switch (gBuiltIns[ival].type) {
					/*  Shift the pushed arguments to the new stack top  */
					case BS_TYPE_INTEGER:
						sVMStackIntPtr[-1] = ((Int *)up)[-1]; break;
					case BS_TYPE_FLOAT:
						/*  Use temporary variable, because the two memory area may
							overlap (e.g. when sizeof(Off_t)=2 and sizeof(Float)=8) */
						dval = ((Float *)up)[-1]; sVMStackFloatPtr[-1] = dval; break;
					case BS_TYPE_STRING: 
						sVMStackOffPtr[-1] = ((Off_t *)up)[-1]; break;
				}
			} else {
				/*  TODO: runtime exception  */
				retval = i3;
				if (retval == -1) {
					/*  Force termination of the exec loop, and fake normal termination */
					retval = 0;
				}
				goto exit;
			}
			break;
			
#pragma mark ------ READ/RESTORE ------
		
		case C_READ_INT:
		case C_READ_FLT:
		case C_READ_STR:
		redo_read:
			ival = bs_dataload_ofs(sVMDataPos + 1);
			i2 = gVMDataBasePtr[sVMDataPos];
			i3 = sVMDataPos + 4 + sDataAccessor.ofs;
			if (i2 == C_DATA) {
				up = gConstStringBasePtr + ival;
				if ((ival = NEXT_DATA(up, sDataAccessor)) == 0) {
					sVMDataPos = i3;
					if (sVMDataPos >= gVMDataTop) {
						bs_runtime_error(MSG_(BS_M_DATA_POS_OUT_OF_RANGE));
						retval = 1;
						goto exit;
					}
					INIT_DATA(sDataAccessor);
					goto redo_read;
				}
				up = gVMDataBasePtr + i3;
				if (ival == BS_TYPE_FLOAT) {
					dval = *((Float *)up);
					if (code == C_READ_FLT)
						goto end_read_flt;
					else if (code == C_READ_INT) {
						ival = (Int)dval;
						goto end_read_int;
					} else {
						bs_runtime_error(MSG_(BS_M_DATA_MISMATCH_NUMBER_EXPECTED));
						retval = 1;
						goto exit;
					}
				} else if (ival == BS_TYPE_INTEGER) {
					ival = *((Int *)up);
					if (code == C_READ_INT)
						goto end_read_int;
					else if (code == C_READ_FLT) {
						dval = (Float)ival;
						goto end_read_flt;
					} else {
						bs_runtime_error(MSG_(BS_M_DATA_MISMATCH_NUMBER_EXPECTED));
						retval = 1;
						goto exit;
					}
				} else {
					sval = *((Off_t *)up);
					if (code != C_READ_STR) {
						bs_runtime_error(MSG_(BS_M_DATA_MISMATCH_STRING_EXPECTED));
						retval = 1;
						goto exit;
					}
					sVMStackOffPtr[0] = sval | kMSBOff;
					sVMStackPtr += sizeof(Off_t);
					break;
				}
			} else if (i2 == C_CDATA) {
				if (sDataAccessor.ofs >= ival) {
					sVMDataPos = i3;
					if (sVMDataPos >= gVMDataTop) {
						bs_runtime_error(MSG_(BS_M_DATA_POS_OUT_OF_RANGE));
						retval = 1;
						goto exit;
					}
					sDataAccessor.ofs = 0;
					goto redo_read;
				}
				if (code != C_READ_INT) {
					bs_runtime_error(MSG_(BS_M_CDATA_READ_ONLY_INTEGER));
					retval = 1;
					goto exit;
				}
				ival = gVMDataBasePtr[i3];
				sDataAccessor.ofs++;
				goto end_read_int;
			} else {
				bs_runtime_error(MSG_(BS_M_BAD_DATA_STATEMENT));
				retval = 1;
				goto exit;
			}
			break;
		end_read_int:
			sVMStackIntPtr[0] = ival;
			sVMStackPtr += sizeof(Int);
			break;
		end_read_flt:
			sVMStackFloatPtr[0] = dval;
			sVMStackPtr += sizeof(Float);
			break;

		case C_RESTORE:
			sVMCodePos = bs_load_progpos(sVMCodePos, &ival);
			sVMDataPos = ival;
			INIT_DATA(sDataAccessor);
			break;
		
		case C_TEST_POS_INT:
			break;

#pragma mark ------ WAIT ------
		case C_PREP_WAIT:
			ival = bs_uptime(0);
			if (gVMCodeBasePtr[sVMCodePos] != C_WAIT) {
				bs_runtime_error(MSG_(BS_M_PREP_WAIT_ERROR));
				retval = 1;
				goto exit;
			}
			sVMStackIntPtr[-1] = ival + sVMStackIntPtr[-1] * 1000;
			sVMCodePos++;
			/*  no break  */
		case C_WAIT:
			ival = sVMStackIntPtr[-1] - (Int)bs_uptime(0);
			if (ival > 0) {
				/*  Wait for some time  */
				if (ival > 1000)
					ival = 1000;
				usleep(ival);
				sVMCodePos--;  /*  Execute this code again  */
			} else {
				sVMStackPtr -= sizeof(Int);
			}
			break;
		case C_STOP:
			retval = 0;
			goto exit;  /*  Normal termination  */
	}
	goto loop;
exit:
	sRunning = 0;
	s_stop_interval_timer();
	return retval;
}
