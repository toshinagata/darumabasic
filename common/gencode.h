/*
 *  gencode.h
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 2015/12/17.
 *  Copyright 2015 Toshi Nagata.  All rights reserved.
 *
 */

#ifndef __gencode_h__
#define __gencode_h__

/*  We assume daruma.h and y.tab.h are already included  */
#ifndef __daruma_h__
#error "daruma.h should be included before gencode.h"
#endif

#if 0
#pragma mark ====== Internal Code ======
#endif

/*  The integer and address operands are described as compressed integers  */
/*  The program positions are described as 24-bit (little-endian) integers  */

enum {
	C_NUL = 0,
	C_NOP,

	/*  Number operations  */
	C_PUSH_INT,      /*  One operand (integer value) */
	C_PUSH_FLT,      /*  One operand (double value)  */
	C_PUSH_STRL,     /*  One operand (string literal ID)  */
	C_PUSH_DIMREF,   /*  One operand (offset to the DIM header; used only for passing DIM to func) */
	C_PUSH_REF,      /*  One operand (offset from gVarBase) */
	C_PUSH_FREF,     /*  One operand (offset from the current frame pointer)  */

	/*  Binary operations: pop two numbers from the stack and push the result  */
	C_ADD_INT,
	C_SUB_INT,
	C_MUL_INT,
	C_DIV_INT,
	C_MOD_INT,
	C_LT_INT,
	C_GT_INT,
	C_LTEQ_INT,
	C_GTEQ_INT,
	C_EQ_INT,
	C_NEQ_INT,
	C_ADD_FLT,
	C_SUB_FLT,
	C_MUL_FLT,
	C_DIV_FLT,
	C_LT_FLT,
	C_GT_FLT,
	C_LTEQ_FLT,
	C_GTEQ_FLT,
	C_EQ_FLT,
	C_NEQ_FLT,
	C_ADD_STR,
	C_LT_STR,
	C_GT_STR,
	C_LTEQ_STR,
	C_GTEQ_STR,
	C_EQ_STR,
	C_NEQ_STR,
	C_AND_INT,
	C_OR_INT,
	C_XOR_INT,
	C_LSHIFT_INT,
	C_RSHIFT_INT,
	
	/*  Unary operations: pop one number from the stack and push the result  */
	C_NEG_INT,
	C_NEG_FLT,
	C_NOT_INT,
	C_FLT_TO_INT,
	C_INT_TO_FLT,
	C_STR_TO_INT,
	C_STR_TO_FLT,
	
	/*  Indirect addressing: pop an address from the stack and push the content of the address */
	C_LOAD_INT,
	C_LOAD_BYTE,
	C_LOAD_FLT,
	C_LOAD_STR,
	C_LOAD_REF,
	
	/*  Indirect addressing: pop an address and a number from the stack and store the number
	    to the address  */
	C_STORE_INT,
	C_STORE_BYTE,
	C_STORE_FLT,
	C_STORE_STR,
	C_STORE_REF,
	
	/*  Indirect addressing from the frame pointer  */
	C_FLOAD_INT,
	C_FLOAD_BYTE,
	C_FLOAD_FLT,
	C_FLOAD_STR,
	C_FLOAD_REF,
	
	/*  Indirect addressing from the frame pointer  */
	C_FSTORE_INT,
	C_FSTORE_BYTE,
	C_FSTORE_FLT,
	C_FSTORE_STR,
	C_FSTORE_REF,

	/*  Special stack operations: remove the value from the stack top and store it
	    in the stack area pointed by the given offset (always negative)
	    Used in building the function/subroutine arguments.  */
	C_SMOVE_INT,
	C_SMOVE_FLT,
	C_SMOVE_STR,
	
	/*  Release the string value in the stack area at the given offset (negative)  */
	C_RELSTR_SP,	
/*	C_RELSTR,   *//*  Release string values in the frame and replace with NULL  */

	/*  Stack operations  */
	C_DUP_REF,  /*  Duplicate the stack top as an offset  */
	C_INC_SP,   /*  One operand: add N to the stack pointer  */

	C_EXCHANGE, /*  Exchange the top two values on the stack; the 1-byte operand
				    denotes the types of the two values; bits 0-2 are for the top
				    value, and bits 3-5 are for the second value. (1:int, 2:flt, 3:ref) */
#if 0
	C_POP,  /*  Throw away the stack top  */
	C_POPN,  /*  One operand: throw away N elements from the stack top  */
	C_PUSH,  /*  Push null to the stack top  */
	C_PUSHN, /*  One operand: push N nulls to the stack top  */
	C_EXCHANGE, /*  Exchange the top two values on the stack  */
#endif
	
	/*  Frame pointer operations  */
	/*  Prepare a new frame: push the frame pointer and copy the current
	    stack pointer to the frame pointer  */
	C_LINK,    /*  One operand  */
	/*  Copy the current frame pointer to the stack pointer,
	 pop the old frame pointer and the old program counter,
	 and jump to the old program counter.  */
	C_RETURN,
	/*  Works similarly as C_RETURN, except that the value on the stack top is
	 kept and pushed to the stack after the old program pointer is restored.  */
	C_RET_INT,
	C_RET_FLT,
	C_RET_STR,

	/*  Branch instructions  */
	/*  Branch always  */
	C_BRA,  /*  One operand (progpos)  */
	/*  Pop an integer value from the stack, and branch if zero  */
	C_BZ,   /*  One operand (progpos)  */
	/*  Pop an integer value from the stack, and branch if non-zero  */
	C_BNZ,  /*  One operand (progpos)  */
	/*  Branch (always) to subroutine  */
	C_BSR,  /*  One operand (progpos)  */
	
	/*  Special compare instructions (to be used in FOR statement)  */
	/*    Frame[N] (N is the operand): address of the control variable  */
	/*    Frame[N+1]: the target value  */
	/*    Frame[N+2]: the step value  */
	/*  Test if the value at Frame[N] 'exceeds' the target value  */
	/*  ('exceeds' means 'greater than' if the step value is positive, and
	    'less than' if the step value is negative. */
	/*  If the step value is zero, then this test never succeeds.  */
	/*  The test result is pushed on the stack as a logical value.  */
	C_FORTEST_INT,  /*  One operand: the frame offset  */
	C_FORTEST_FLT,  /*  One operand: the frame offset  */
	/*  The value at Frame[N] is increased (or decreased) by the step value,
	    then test the same way as FORTEST above */
	C_NEXT_INT,     /*  One operand: the frame offset  */
	C_NEXT_FLT,     /*  One operand: the frame offset  */
	/*  Same as above, except that the variable is local (i.e. in the frame)  */
	C_FORTEST_INTL,
	C_FORTEST_FLTL,
	C_NEXT_INTL,
	C_NEXT_FLTL,
	
	/*  I/O instructions  */
	C_NEWLINE,
	C_PUTC, /*  One operand: character code  */
	C_PRINT_INT,   /*  Print stack top as an integer  */
	C_PRINT_FLT,   /*  Print stack top as a float  */
	C_PRINT_STR,   /*  Print stack top as a string  */
	C_INPUT,       /*  Input a single line and push it as a string  */
	
	/*  DIM-related instructions  */
	/*  DEF_DIM_XXX: pop the dimension size information from the stack, set to the
	    runtime DimInfo structure, and allocate memory.  */
	/*  Stack[-1]: dimension=N, Stack[-2]: size N-1, Stack[-3]: size N-2, ..., Stack[-N-1]: size 0 */
	/*  One operand: index to DimInfo  */
	/*  Throws exception if any of the dimension size is non-positive or malloc() fails  */
	C_DEF_DIM_INT,
	C_DEF_DIM_FLT,
	C_DEF_DIM_STR,
	C_DEF_DIM_BYTE,
	/*  GET_DIM_XXX: calc the address of the dimension element and push to the stack.
	 The address is calculated from the index to gDimInfos (stack top) and the
	 arguments on the stack  */
	C_GET_DIM_INT,
	C_GET_DIM_FLT,
	C_GET_DIM_STR,
	C_GET_DIM_BYTE,

	/*  DIM initializer support  */
	C_PREP_DIM,
	C_STORELOOP_INT,
	C_STORELOOP_FLT,
	C_STORELOOP_STR,
	C_STORELOOP_BYTE,
	
	/*  Built-in functions  */
	/*  The arguments and the number of arguments are on the stack.  */
	/*  The number of arguments is popped from the stack, and given as an argument
	    to the built-in function handler.
	    The function handler accesses to the arguments as Stack[-n]...Stack[-1] (arg1...n).
	    If the function returns value, then it is placed in Stack[0].
	    After returning from the function handler, the internal code interpreter removes
	    the arguments from the stack and push the return value to the stack.  */
	/*  The operand is the index to gBuiltInEntries  */
	C_BUILTIN_FUNC, /*  With a return value  */
	C_BUILTIN_SUB,  /*  Without a return value  */

	/*  READ/RESTORE statements  */
	C_READ_INT,
	C_READ_FLT,
	C_READ_STR,
	C_RESTORE,      /*  One operand (progpos)  */
	
	/*  DATA/CDATA statements (only appears in the DATA section)  */
	C_DATA,         /*  Begin data  */
	C_CDATA,        /*  Begin byte data  */
	
	/*  Runtime exceptions  */
	C_TEST_POS_INT, /*  Throw exception if the stack top is non-positive */
	
	/*  Wait  */
	C_PREP_WAIT,    /*  Push the 'wait end' time to the stack  */
	C_WAIT,         /*  Halt until the 'wait end' time is reached  */
	
	C_STOP  /*  Terminate execution  */
};


/*  Global variable to keep the jump address during compilation of
    IF/DO/FOR/RETURN(END SUB) statement  */
/*  The BRA/BZ statements are linked by use of the address field  */
/*extern int gIfLink;
extern int gDoLink;
extern int gForLink;
extern int gReturnLink; */

extern void bs_dump_vmcode(Off_t pos, int n);  /*  For debug  */
extern int bs_get_lineno_for_vmcodepos(Off_t pos);

extern Off_t bs_store_operand(Off_t pos, int32_t val);
extern Off_t bs_load_operand(Off_t pos, int32_t *outval);
extern Off_t bs_store_progpos(Off_t pos, Off_t progpos);
extern Off_t bs_load_progpos(Off_t pos, Off_t *outprogpos);
extern Float bs_load_float(Off_t pos);
extern Off_t bs_load_string(Off_t pos, Off_t *val);

extern void bs_code0(u_int8_t code);
extern void bs_code1(u_int8_t code, int32_t operand);
extern Off_t bs_code2(u_int8_t code, Off_t progpos);
extern void bs_code3(u_int8_t code, Float fval);
extern void bs_code4(u_int8_t code, Off_t str);

extern void bs_datagen_byte(u_int8_t byte);
extern void bs_datagen_integer(Int ival);
extern void bs_datagen_float(Float dval);
extern void bs_datagen_str(Off_t str);
extern void bs_datagen_ofs(Off_t str);
extern u_int8_t bs_dataload_byte(Off_t pos);
extern Int bs_dataload_integer(Off_t pos);
extern Float bs_dataload_float(Off_t pos);
extern Off_t bs_dataload_str(Off_t pos);
extern Off_t bs_dataload_ofs(Off_t pos);

extern int bs_generate_const_expr(TokenValue *tp);
extern int bs_generate_const_op1(TokenValue *tpd, const TokenValue *tp1, int op);
extern int bs_generate_const_op2(TokenValue *tpd, const TokenValue *tp1, const TokenValue *tp2, int op);
extern int bs_generate_const_int_op2(TokenValue *tpd, const TokenValue *tp1, const TokenValue *tp2, int op);

extern int bs_generate_op1(int vtype1, int op);
extern int bs_generate_op2(int vtype1, int vtype2, int op);
extern int bs_generate_int_op2(int vtype1, int vtype2, int op);

extern int bs_generate_coerce(int new_type, int old_type);
extern int bs_generate_var_ref(int varidx);
extern int bs_generate_store(int store_type, int value_type);
extern int bs_generate_load(int value_type);

extern int bs_check_outside_control_statement(const char *msg);

extern int bs_generate_def_dim(int nargs, int dimidx);
extern int bs_generate_get_dim(int nargs, int dimidx);

extern int bs_start_loop(int type);
extern int bs_complete_for_header(int var_type, int stype);
extern void bs_resolve_links(Off_t *link, Off_t addr);
extern int bs_generate_break_statement(void);
extern int bs_generate_continue_statement(void);
extern int bs_finalize_loop(int save_loopidx);
extern int bs_generate_do_cond(int stype);

extern int bs_make_conditional_expr(int stype);
extern void bs_complete_if_statement(void);
extern int bs_generate_goto(int labelidx);
extern int bs_check_goto_with_for_nesting_level(int pos, int is_backward);
extern int bs_define_label(int labelidx);
extern int bs_check_undefined_labels(int scope);
extern int bs_check_undefined_funcs(void);

extern int bs_start_funcall(int funcidx, int is_func);
extern int bs_check_argument(int stype, int dim_index);
extern int bs_finalize_funcall(int save_tempsig);

extern int bs_start_funcdef(int funcidx, int is_func);
extern int bs_check_formal_argument(int varidx);
extern int bs_close_formal_argument_list(void);
extern int bs_finalize_funcdef(int is_func);
extern int bs_generate_return(int stype);

extern int bs_start_local_statement(void);
extern int bs_check_local_var(int varidx);
extern int bs_finalize_local_statement(void);

extern int bs_start_data_statement(u_int8_t code);
extern int bs_generate_data(TokenValue *tp);
extern int bs_generate_cdata(TokenValue *tp);
extern int bs_close_data_statement(void);
extern int bs_generate_restore(int labelidx);
extern int bs_generate_read_and_store(int store_type);

extern int bs_generate_input(int store_type);

#if 0
#pragma mark ====== Execution ======
#endif

extern int bs_execinit(Off_t progEntry, int new_run);
extern int bs_execcode(void);
extern Off_t bs_codepos(void);
extern int bs_is_running(void);

#endif /* __gencode_h__ */
