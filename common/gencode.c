/*
 *  gencode.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 2016/01/17.
 *  Copyright 2016 Toshi Nagata.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "y.tab.h"
#include "daruma.h"
#include "gencode.h"
#include "transmessage.h"

static const struct {
	const char *opecode;
	int operand;  /*  0: no operand, 1: one integer operand,
				      2: one address operand, 3: one progpos operand,
				      4: one float operand */
} sOpTable[] = {
	{ "NUL", 0 },
	{ "NOP", 0 },
	{ "PUSH_INT", 1 },
	{ "PUSH_FLT", 4 },
	{ "PUSH_STRL", 1 },
	{ "PUSH_DIMREF", 1 },
	{ "PUSH_REF", 1 },
	{ "PUSH_FREF", 1 },
	{ "ADD_INT", 0 },
	{ "SUB_INT", 0 },
	{ "MUL_INT", 0 },
	{ "DIV_INT", 0 },
	{ "MOD_INT", 0 },
	{ "LT_INT", 0 },
	{ "GT_INT", 0 },
	{ "LTEQ_INT", 0 },
	{ "GTEQ_INT", 0 },
	{ "EQ_INT", 0 },
	{ "NEQ_INT", 0 },
	{ "ADD_FLT", 0 },
	{ "SUB_FLT", 0 },
	{ "MUL_FLT", 0 },
	{ "DIV_FLT", 0 },
	{ "LT_FLT", 0 },
	{ "GT_FLT", 0 },
	{ "LTEQ_FLT", 0 },
	{ "GTEQ_FLT", 0 },
	{ "EQ_FLT", 0 },
	{ "NEQ_FLT", 0 },
	{ "ADD_STR", 0 },
	{ "LT_STR", 0 },
	{ "GT_STR", 0 },
	{ "LTEQ_STR", 0 },
	{ "GTEQ_STR", 0 },
	{ "EQ_STR", 0 },
	{ "NEQ_STR", 0 },
	{ "AND_INT", 0 },
	{ "OR_INT", 0 },
	{ "XOR_INT", 0 },
	{ "LSHIFT_INT", 0 },
	{ "RSHIFT_INT", 0 },
	{ "NEG_INT", 0 },
	{ "NEG_FLT", 0 },
	{ "NOT_INT", 0 },
	{ "FLT_TO_INT", 0 },
	{ "INT_TO_FLT", 0 },
	{ "LOAD_INT", 0 },
	{ "LOAD_BYTE", 0 },
	{ "LOAD_FLT", 0 },
	{ "LOAD_STR", 0 },
	{ "LOAD_REF", 0 },
	{ "STORE_INT", 0 },
	{ "STORE_BYTE", 0 },
	{ "STORE_FLT", 0 },
	{ "STORE_STR", 0 },
	{ "STORE_REF", 0 },
	{ "FLOAD_INT", 0 },
	{ "FLOAD_BYTE", 0 },
	{ "FLOAD_FLT", 0 },
	{ "FLOAD_STR", 0 },
	{ "FLOAD_REF", 0 },
	{ "FSTORE_INT", 0 },
	{ "FSTORE_BYTE", 0 },
	{ "FSTORE_FLT", 0 },
	{ "FSTORE_STR", 0 },
	{ "FSTORE_REF", 0 },
	{ "SMOVE_INT", 1 },
	{ "SMOVE_FLT", 1 },
	{ "SMOVE_STR", 1 },
	{ "RELSTR_SP", 1 },
	{ "RELSTR", 1 },
	{ "DUP_REF", 0 },
	{ "INC_SP", 1 },
	{ "POP", 0 },
	{ "POPN", 1 },
	{ "PUSH", 0 },
	{ "PUSHN", 1 },
	{ "EXCHANGE", 0 },
	{ "LINK", 1 },
	{ "RETURN", 0 },
	{ "RET_INT", 0 },
	{ "RET_FLT", 0 },
	{ "RET_STR", 0 },
	{ "BRA", 3 },
	{ "BZ", 3 },
	{ "BNZ", 3 },
	{ "BSR", 3 },
	{ "FORTEST_INT", 1 },
	{ "FORTEST_FLT", 1 },
	{ "NEXT_INT", 1 },
	{ "NEXT_FLT", 1 },
	{ "FORTEST_INTL", 1 },
	{ "FORTEST_FLTL", 1 },
	{ "NEXT_INTL", 1 },
	{ "NEXT_FLTL", 1 },
	{ "NEWLINE", 0 },
	{ "PUTC", 1 },
	{ "PRINT_INT", 0 },
	{ "PRINT_FLT", 0 },
	{ "PRINT_STR", 0 },
	{ "DEF_DIM_INT", 1 },
	{ "DEF_DIM_FLT", 1 },
	{ "DEF_DIM_STR", 1 },
	{ "DEF_DIM_BYTE", 1 },
	{ "GET_DIM_INT", 1 },
	{ "GET_DIM_FLT", 1 },
	{ "GET_DIM_STR", 1 },
	{ "GET_DIM_BYTE", 1 },
	{ "PREP_DIM", 1 },
	{ "STORELOOP_INT", 0 },
	{ "STORELOOP_FLT", 0 },
	{ "STORELOOP_STR", 0 },
	{ "STORELOOP_BYTE", 0 },
	{ "BUILTIN_FUNC", 1 },
	{ "BUILTIN_SUB", 1 },
	{ "READ_INT", 0 },
	{ "READ_FLT", 0 },
	{ "READ_STR", 0 },
	{ "RESTORE", 3 },
	{ "DATA", 3 },
	{ "CDATA", 3 },
	{ "TEST_POS_INT", 0 },
	{ "PREP_WAIT", 0 },
	{ "WAIT", 0 },
	{ "STOP", 0 },
	NULL  /*  Sentinel  */
};

int gIfLink, gDoLink, gForLink, gReturnLink;

/*  Look for the line number from VMCode address (assuming gVMAddress[] table is valid) */
/*  Lineno is 1-based  */
int
bs_get_lineno_for_vmcodepos(Off_t pos)
{
	int i;
	if (pos < 0 || pos >= gVMCodeTop)
		return -1;
	for (i = 0; i < gVMAddressTop / sizeof(Off_t); i++) {
		if (((Off_t *)gVMAddressBasePtr)[i] > pos)
			return i;
	}
	return i;
}

/*  Get the VMCode address from the line number (1-based)  */
Off_t
bs_get_vmcodepos_for_lineno(int lineno)
{
	if (lineno <= 0 || lineno > gVMAddressTop / sizeof(Off_t))
		return kInvalidProgPos;
	return ((Off_t *)gVMAddressBasePtr)[lineno - 1];
}

/*  Disassemble internal code  */
void
bs_dump_vmcode(Off_t pos, int n)
{
	Int c, val;

	if (n <= 0)
		n = 100;

	while (--n >= 0 && (c = gVMCodeBasePtr[pos]) != C_NUL) {
		Float dval;
		pos++;
		debug_printf("%06X: %s", pos - 1, sOpTable[c].opecode);
		switch (sOpTable[c].operand) {
			case 1:
				pos = bs_load_operand(pos, &val);
				debug_printf(" %d", val);
				break;
			case 2:
				pos = bs_load_operand(pos, &val);
				debug_printf(" 0x%08X", (u_int32_t)(val));
				break;
			case 3:
				pos = bs_load_progpos(pos, &val);
				debug_printf(" %06X", val);
				break;
			case 4:
				dval = bs_load_float(pos);
				debug_printf(" %#g", dval);
				pos += sizeof(Float);
				break;
		}
		debug_printf("\n");
	}
}

/*  Disassemble Data section  */
void
bs_dump_vmdata(Off_t pos, int n)
{
	DataAccessor acc;
	u_int8_t c;
	u_int8_t *sig;

	if (n <= 0)
		n = 100;
	n += pos;
	if (n > gVMDataTop)
		n = gVMDataTop;
	if (gVMDataTop == 0)
		debug_printf("--- NO DATA SECTION ---\n");
	else
		debug_printf("      .Data\n");
	while (pos < n) {
		Off_t ofs;
		int i;
		c = gVMDataBasePtr[pos++];
		ofs = bs_dataload_ofs(pos);
		pos += 3;
		if (c == C_DATA) {
			debug_printf("%06X: DATA %d\n", pos - 4, ofs);
			sig = gConstStringBasePtr + ofs;
			INIT_DATA(acc);
			while (pos < n && (i = NEXT_DATA(sig, acc)) > 0) {
				switch (i) {
					case 1:
						debug_printf("%06X: INT %d\n", pos, bs_dataload_integer(pos));
						pos += sizeof(Int);
						break;
					case 2:
						debug_printf("%06X: FLOAT %#g\n", pos, bs_dataload_float(pos));
						pos += sizeof(Float);
						break;
					case 3:
						debug_printf("%06X: STRL %s\n", pos, bs_get_string_ptr(bs_dataload_str(pos)));
						pos += sizeof(Off_t);
						break;
				}
			}
		} else if (c == C_CDATA) {
			debug_printf("%06X: CDATA %d\n", pos - 4, ofs);
			i = 0;
			while (pos < n && i < ofs) {
				if (i % 8 == 0)
					printf("%06X: BYTE ", pos);
				debug_printf("%d%c", gVMDataBasePtr[pos],
					   (i == ofs - 1 || pos == n - 1 || i % 8 == 7 ? '\n' : ','));
				i++;
				pos++;
			}
		} else {
			debug_printf("%06X: Unknown data identifier %d\n", pos - 4, c);
			break;
		}
	}		
}

#if 0
#pragma mark ====== Common Functions ======
#endif

/*  7*N bits signed integer; little endian, and the last byte has
    MSB = 1  */
Off_t
bs_store_operand(Off_t pos, int32_t val)
{
	gVMCodeBasePtr[pos++] = (val & 0x7f);
	if (val < -0x40 || val > 0x3f) {
		gVMCodeBasePtr[pos++] = ((val >> 7) & 0x7f);
		if (val < -0x2000 || val > 0x1fff) {
			gVMCodeBasePtr[pos++] = ((val >> 14) & 0x7f);
			if (val < -0x100000 || val > 0xfffff) {
				gVMCodeBasePtr[pos++] = ((val >> 21) & 0x7f);
				if (val < -0x8000000) {
					gVMCodeBasePtr[pos++] = ((val >> 28) & 0x7f) | 0x70;
				} else {
					gVMCodeBasePtr[pos++] = ((val >> 28) & 0x7f);
				}
			}
		}
	}
	gVMCodeBasePtr[pos - 1] |= 0x80;
	return pos;
}

Off_t
bs_load_operand(Off_t pos, int32_t *outval)
{
	int32_t val = 0;
	int n = 0;
	while (1) {
		u_int8_t c = gVMCodeBasePtr[pos];
		if (c & 0x80) {
			/*  Extend the sign from bit 6  */
			val |= ((int32_t)((int8_t)(c << 1)) / 2) << n;
			break;
		} else {
			val |= ((u_int32_t)c) << n;
		}
		pos++;
		n += 7;
	}
	*outval = val;
	return pos + 1;
}

Off_t
bs_store_progpos(Off_t pos, Off_t progpos)
{
	gVMCodeBasePtr[pos++] = progpos & 0xff;
	gVMCodeBasePtr[pos++] = (progpos >> 8) & 0xff;
#if BS_OFF_T_IS_32BIT
	gVMCodeBasePtr[pos++] = (progpos >> 16) & 0xff;
#endif
	return pos;
}

Off_t
bs_load_progpos(Off_t pos, Off_t *outprogpos)
{
	Off_t progpos;
	progpos = gVMCodeBasePtr[pos++];
	progpos += ((Off_t)gVMCodeBasePtr[pos++]) << 8;
#if BS_OFF_T_IS_32BIT
	progpos += ((Off_t)gVMCodeBasePtr[pos++]) << 16;
#endif
	*outprogpos = progpos;
	return pos;
}

Float
bs_load_float(Off_t pos)
{
	Float fval;
	((u_int8_t *)&fval)[0] = gVMCodeBasePtr[pos++];
	((u_int8_t *)&fval)[1] = gVMCodeBasePtr[pos++];
	((u_int8_t *)&fval)[2] = gVMCodeBasePtr[pos++];
	((u_int8_t *)&fval)[3] = gVMCodeBasePtr[pos++];
#if SIZEOF_FLOAT == 8
	((u_int8_t *)&fval)[4] = gVMCodeBasePtr[pos++];
	((u_int8_t *)&fval)[5] = gVMCodeBasePtr[pos++];
	((u_int8_t *)&fval)[6] = gVMCodeBasePtr[pos++];
	((u_int8_t *)&fval)[7] = gVMCodeBasePtr[pos++];
#endif
	return fval;
}

Off_t
bs_load_string(Off_t pos, Off_t *val)
{
	int32_t ival;
	pos = bs_load_operand(pos, &ival);
	*val = ival;
	return pos;
}

#if 0
#pragma mark ====== Generating Code ======
#endif

void
bs_code0(u_int8_t code)
{
	gVMCodeBasePtr[gVMCodeTop++] = code;
}

void
bs_code1(u_int8_t code, int32_t operand)
{
	gVMCodeBasePtr[gVMCodeTop++] = code;
	gVMCodeTop = bs_store_operand(gVMCodeTop, operand);
}

/*  The position of the progpos field is returned (used in compilation)  */
Off_t
bs_code2(u_int8_t code, Off_t progpos)
{
	Off_t retval;
	gVMCodeBasePtr[gVMCodeTop++] = code;
	retval = gVMCodeTop;
	gVMCodeTop = bs_store_progpos(gVMCodeTop, progpos);
	return retval;
}

void
bs_code3(u_int8_t code, Float dval)
{
	gVMCodeBasePtr[gVMCodeTop++] = code;
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[0];
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[1];
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[2];
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[3];
#if SIZEOF_FLOAT == 8
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[4];
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[5];
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[6];
	gVMCodeBasePtr[gVMCodeTop++] = ((u_int8_t *)&dval)[7];
#endif
}

/*  Generate code with string operand  */
void
bs_code4(u_int8_t code, Off_t str)
{
	bs_code1(code, str & ~kMSBOff);
}

/*  Generate code in the data section  */
void
bs_datagen_byte(u_int8_t byte)
{
	gVMDataBasePtr[gVMDataTop++] = byte;
}

void
bs_datagen_integer(Int ival)
{
	gVMDataBasePtr[gVMDataTop++] = (ival & 0xff);
	gVMDataBasePtr[gVMDataTop++] = ((ival >> 8) & 0xff);
	gVMDataBasePtr[gVMDataTop++] = ((ival >> 16) & 0xff);
	gVMDataBasePtr[gVMDataTop++] = ((ival >> 24) & 0xff);
}

void
bs_datagen_float(Float dval)
{
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[0];
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[1];
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[2];
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[3];
#if SIZEOF_FLOAT == 8
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[4];
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[5];
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[6];
	gVMDataBasePtr[gVMDataTop++] = ((u_int8_t *)&dval)[7];
#endif
}

void
bs_datagen_ofs(Off_t ofs)
{
	gVMDataBasePtr[gVMDataTop++] = (ofs & 0xff);
	gVMDataBasePtr[gVMDataTop++] = ((ofs >> 8) & 0xff);
#if BS_OFF_T_IS_32BIT
	/*  Store only 24 bits (same as progpos)  */
	gVMDataBasePtr[gVMDataTop++] = ((ofs >> 16) & 0xff);
#endif	
}

void
bs_datagen_str(Off_t str)
{
	bs_datagen_ofs(str & ~kMSBOff);
}

u_int8_t
bs_dataload_byte(Off_t pos)
{
	return gVMDataBasePtr[pos];
}

Int
bs_dataload_integer(Off_t pos)
{
	Int ival;
	ival = gVMDataBasePtr[pos++];
	ival |= ((Int)gVMDataBasePtr[pos++]) << 8;
	ival |= ((Int)gVMDataBasePtr[pos++]) << 16;
	ival |= ((Int)gVMDataBasePtr[pos++]) << 24;
	return ival;
}

Float
bs_dataload_float(Off_t pos)
{
	return bs_load_float(pos + (gVMDataBasePtr - gVMCodeBasePtr));
}

Off_t
bs_dataload_ofs(Off_t pos)
{
	Off_t str;
	str = gVMDataBasePtr[pos++];
	str |= ((Int)gVMDataBasePtr[pos++]) << 8;
#if BS_OFF_T_IS_32BIT
	str |= ((Int)gVMDataBasePtr[pos++]) << 16;
#endif
	return str;
}

Off_t
bs_dataload_str(Off_t pos)
{
	return bs_dataload_ofs(pos) | kMSBOff;
}

#if 0
#pragma mark ====== Constant Expression ======
#endif

int
bs_generate_const_expr(TokenValue *tp)
{
	if (tp->type == BS_TYPE_INTEGER)
		bs_code1(C_PUSH_INT, tp->u.iv);
	else if (tp->type == BS_TYPE_FLOAT)
		bs_code3(C_PUSH_FLT, tp->u.fv);
	else if (tp->type == BS_TYPE_STRING)
		bs_code4(C_PUSH_STRL, tp->u.sv);
	else {
		bs_error(MSG_(BS_M_BAD_CONST_EXPR), tp->type);
		return -1;
	}
	return tp->type;
}

int
bs_generate_const_op1(TokenValue *tpd, const TokenValue *tp1, int op)
{
	int type = tp1->type;
	if (op == '-') {
		if (type == BS_TYPE_INTEGER)
			tpd->u.iv = -tp1->u.iv;
		else if (type == BS_TYPE_FLOAT)
			tpd->u.fv = -tp1->u.fv;
		else goto type_mismatch;
	} else if (op == '+') {
		if (type != BS_TYPE_INTEGER && type != BS_TYPE_FLOAT)
			goto type_mismatch;
		tpd->u = tp1->u;
	} else if (op == BS_NOT) {
		if (type != BS_TYPE_INTEGER)
			goto type_mismatch;
		tpd->u.iv = (tp1->u.iv != 0);
	} else {
		bs_error(MSG_(BS_M_UNKNOWN_UNARY), op);
		return -1;
	}
	tpd->type = type;
	return type;
type_mismatch:
	bs_error(MSG_(BS_M_NUMBER_EXPECTED));
	return -1;
}

int
bs_generate_const_op2(TokenValue *tpd, const TokenValue *tp1, const TokenValue *tp2, int op)
{
	int vtype1 = tp1->type;
	int vtype2 = tp2->type;
	TokenValue t1 = *tp1;
	TokenValue t2 = *tp2;
	if (vtype1 == BS_TYPE_FLOAT && vtype2 == BS_TYPE_INTEGER) {
		/*  Promote int to float (second operand)  */
		t2.u.fv = (Float)t2.u.iv;
		vtype2 = BS_TYPE_FLOAT;
	} else if (vtype1 == BS_TYPE_INTEGER && vtype2 == BS_TYPE_FLOAT) {
		/*  Promote int to float (first operand)  */ 
		t1.u.fv = (Float)t1.u.fv;
		vtype1 = BS_TYPE_FLOAT;
	}
	if (vtype1 != vtype2) {
		bs_error(MSG_(BS_M_TYPE_MISMATCH));
		return -1;
	}
	if (op == '+' || op == '-' || op == '*' || op == '/') {
		if (vtype1 == BS_TYPE_STRING) {
			const char *p1, *p2;
			if (op != '+') {
				bs_error(MSG_(BS_M_ILLEGAL_OP_STR), op);
				return -1;
			}
			p1 = bs_get_string_ptr(t1.u.sv);
			p2 = bs_get_string_ptr(t2.u.sv);
			tpd->u.sv = bs_allocate_literal_string(p1, strlen(p1) + strlen(p2));
			strcat(bs_get_string_ptr(tpd->u.sv), p2);
			tpd->type = vtype1;
			return vtype1;
		}
		if (vtype1 == BS_TYPE_INTEGER) {
			switch (op) {
				case '+': tpd->u.iv = t1.u.iv + t2.u.iv; break;
				case '-': tpd->u.iv = t1.u.iv - t2.u.iv; break;
				case '*': tpd->u.iv = t1.u.iv * t2.u.iv; break;
				case '/': tpd->u.iv = t1.u.iv / t2.u.iv; break;
			}
		} else if (vtype1 == BS_TYPE_FLOAT) {
			switch (op) {
				case '+': tpd->u.fv = t1.u.fv + t2.u.fv; break;
				case '-': tpd->u.fv = t1.u.fv - t2.u.fv; break;
				case '*': tpd->u.fv = t1.u.fv * t2.u.fv; break;
				case '/': tpd->u.fv = t1.u.fv / t2.u.fv; break;
			}
		} else {
			bs_error(MSG_(BS_M_NUMBER_EXPECTED));
			return -1;
		}
		tpd->type = vtype1;
		return vtype1;
	} else if (op == '>' || op == '<' || op == '=' || op == BS_NEQ || op == BS_GTEQ || op == BS_LTEQ) {
		if (vtype1 == BS_TYPE_STRING) {
			/*  Not implemented yet  */
		} else if (vtype1 == BS_TYPE_INTEGER) {
			switch (op) {
				case '>': tpd->u.iv = (t1.u.iv > t2.u.iv); break;
				case '=': tpd->u.iv = (t1.u.iv == t2.u.iv); break;
				case '<': tpd->u.iv = (t1.u.iv < t2.u.iv); break;
				case BS_NEQ: tpd->u.iv = (t1.u.iv != t2.u.iv); break;
				case BS_GTEQ: tpd->u.iv = (t1.u.iv >= t2.u.iv); break;
				case BS_LTEQ: tpd->u.iv = (t1.u.iv <= t2.u.iv); break;
			}
		} else if (vtype1 == BS_TYPE_FLOAT) {
			switch (op) {
				case '>': tpd->u.iv = (t1.u.fv > t2.u.fv); break;
				case '=': tpd->u.iv = (t1.u.fv == t2.u.fv); break;
				case '<': tpd->u.iv = (t1.u.fv < t2.u.fv); break;
				case BS_NEQ: tpd->u.iv = (t1.u.fv != t2.u.fv); break;
				case BS_GTEQ: tpd->u.iv = (t1.u.fv >= t2.u.fv); break;
				case BS_LTEQ: tpd->u.iv = (t1.u.fv <= t2.u.fv); break;
			}
		}
		tpd->type = BS_TYPE_INTEGER;
		return BS_TYPE_INTEGER;
	} else {
		bs_error(MSG_(BS_M_UNKNOWN_BINARY), op);
		return -1;
	}
}

int
bs_generate_const_int_op2(TokenValue *tpd, const TokenValue *tp1, const TokenValue *tp2, int op)
{
	int vtype1 = tp1->type;
	int vtype2 = tp2->type;
	if (vtype1 != BS_TYPE_INTEGER || vtype2 != BS_TYPE_INTEGER) {
		bs_error(MSG_(BS_M_MUST_BE_INTEGER));
		return -1;
	}
	switch (op) {
		case '%': tpd->u.iv = (tp1->u.iv % tp1->u.iv); break;
		case BS_AND: tpd->u.iv = (tp1->u.iv & tp1->u.iv); break;
		case BS_OR:  tpd->u.iv = (tp1->u.iv | tp1->u.iv); break;
		case BS_XOR: tpd->u.iv = (tp1->u.iv ^ tp1->u.iv); break;
		case BS_LSHIFT: tpd->u.iv = (tp1->u.iv << tp1->u.iv); break;
		case BS_RSHIFT: tpd->u.iv = (tp1->u.iv >> tp1->u.iv); break;
		default:
			bs_error(MSG_(BS_M_UNKNOWN_INT_BINARY), op);
			return -1;
	}
	tpd->type = BS_TYPE_INTEGER;
	return BS_TYPE_INTEGER;
}


#if 0
#pragma mark ====== Unary and Binary Operations ======
#endif

int
bs_generate_op1(int vtype1, int op)
{
	if (op == '-') {
		if (vtype1 == BS_TYPE_INTEGER)
			bs_code0(C_NEG_INT);
		else if (vtype1 == BS_TYPE_FLOAT)
			bs_code0(C_NEG_FLT);
		else goto type_mismatch;
	} else if (op == '+') {
		if (vtype1 != BS_TYPE_INTEGER && vtype1 != BS_TYPE_FLOAT)
			goto type_mismatch;
	} else if (op == BS_NOT) {
		if (vtype1 != BS_TYPE_INTEGER)
			goto type_mismatch;
		bs_code0(C_NOT_INT);
	} else {
		bs_error(MSG_(BS_M_UNKNOWN_UNARY), op);
		return -1;
	}
	return vtype1;
type_mismatch:
	bs_error(MSG_(BS_M_NUMBER_EXPECTED));
	return -1;
}

int
bs_generate_op2(int vtype1, int vtype2, int op)
{
	if (vtype1 == BS_TYPE_FLOAT && vtype2 == BS_TYPE_INTEGER) {
		/*  Promote int to float (second operand)  */
		bs_code0(C_INT_TO_FLT);
		vtype2 = BS_TYPE_FLOAT;
	} else if (vtype1 == BS_TYPE_INTEGER && vtype2 == BS_TYPE_FLOAT) {
		/*  Promote int to float (first operand)  */ 
		bs_code0(C_EXCHANGE);
		bs_code0(C_INT_TO_FLT);
		bs_code0(C_EXCHANGE);
		vtype1 = BS_TYPE_FLOAT;
	}
	if (vtype1 != vtype2) {
		bs_error(MSG_(BS_M_TYPE_MISMATCH));
		return -1;
	}
	if (op == '+' || op == '-' || op == '*' || op == '/') {
		if (vtype1 == BS_TYPE_STRING) {
			if (op != '+') {
				bs_error(MSG_(BS_M_ILLEGAL_OP_STR), op);
				return -1;
			}
			bs_code0(C_ADD_STR);
			return vtype1;
		}
		if (vtype1 != BS_TYPE_INTEGER && vtype1 != BS_TYPE_FLOAT) {
			bs_error(MSG_(BS_M_NUMBER_EXPECTED));
			return -1;
		}
		switch (op) {
			case '+': bs_code0(vtype1 == BS_TYPE_INTEGER ? C_ADD_INT : C_ADD_FLT); break;
			case '-': bs_code0(vtype1 == BS_TYPE_INTEGER ? C_SUB_INT : C_SUB_FLT); break;
			case '*': bs_code0(vtype1 == BS_TYPE_INTEGER ? C_MUL_INT : C_MUL_FLT); break;
			case '/': bs_code0(vtype1 == BS_TYPE_INTEGER ? C_DIV_INT : C_DIV_FLT); break;
		}
		return vtype1;
	} else if (op == '>' || op == '<' || op == '=' || op == BS_NEQ || op == BS_GTEQ || op == BS_LTEQ) {
		if (vtype1 == BS_TYPE_STRING) {
			/*  Not implemented yet  */
		} else {
			switch (op) {
				case '>':
					bs_code0(vtype1 == BS_TYPE_INTEGER ? C_GT_INT : C_GT_FLT); break;
				case '<':
					bs_code0(vtype1 == BS_TYPE_INTEGER ? C_LT_INT : C_LT_FLT); break;
				case '=':
					bs_code0(vtype1 == BS_TYPE_INTEGER ? C_EQ_INT : C_EQ_FLT); break;
				case BS_NEQ:
					bs_code0(vtype1 == BS_TYPE_INTEGER ? C_NEQ_INT : C_NEQ_FLT); break;
				case BS_GTEQ:
					bs_code0(vtype1 == BS_TYPE_INTEGER ? C_GTEQ_INT : C_GTEQ_FLT); break;
				case BS_LTEQ:
					bs_code0(vtype1 == BS_TYPE_INTEGER ? C_LTEQ_INT : C_LTEQ_FLT); break;
			}
		}
		return BS_TYPE_INTEGER;
	} else {
		bs_error(MSG_(BS_M_UNKNOWN_BINARY), op);
		return -1;
	}
}

int
bs_generate_int_op2(int vtype1, int vtype2, int op)
{
	if (vtype1 != BS_TYPE_INTEGER || vtype2 != BS_TYPE_INTEGER) {
		bs_error(MSG_(BS_M_MUST_BE_INTEGER));
		return -1;
	}
	switch (op) {
		case '%': bs_code0(C_MOD_INT); break;
		case BS_AND: bs_code0(C_AND_INT); break;
		case BS_OR:  bs_code0(C_OR_INT); break;
		case BS_XOR: bs_code0(C_XOR_INT); break;
		case BS_LSHIFT: bs_code0(C_LSHIFT_INT); break;
		case BS_RSHIFT: bs_code0(C_RSHIFT_INT); break;
		default:
			bs_error(MSG_(BS_M_UNKNOWN_INT_BINARY), op);
			return -1;
	}
	return BS_TYPE_INTEGER;
}


#if 0
#pragma mark ====== Variables =======
#endif

/*  Generate code for pushing the variable address to the stack  */
int
bs_generate_var_ref(int varidx)
{
	VarInfo *vp = ((VarInfo *)gVarInfoBasePtr) + varidx;
	u_int8_t c = (vp->scope > 0 ? C_PUSH_FREF : C_PUSH_REF);
	switch (vp->code.type) {
		case BS_TYPE_INTEGER:
		case BS_TYPE_FLOAT:
		case BS_TYPE_STRING:
			bs_code1(c, vp->offset);
			break;
		default:
			bs_error(MSG_(BS_M_INTERNAL_ERROR_1), vp->code.type);
			return -1;
	}
	return vp->code.type + (vp->scope > 0 ? BS_TYPE_LOCAL : 0);
}

/*  Change the type of the stack top  */
int
bs_generate_coerce(int new_type, int old_type)
{
	if (new_type == BS_TYPE_FLOAT) {
		if (old_type == BS_TYPE_INTEGER) {
			bs_code0(C_INT_TO_FLT);
		} else if (old_type != BS_TYPE_FLOAT) {
			bs_error(MSG_(BS_M_CANNOT_CONVERT_FLT));
			return -1;
		}
	} else if (new_type == BS_TYPE_INTEGER) {
		if (old_type == BS_TYPE_FLOAT) {
			bs_code0(C_FLT_TO_INT);
		} else if (old_type != BS_TYPE_INTEGER) {
			bs_error(MSG_(BS_M_CANNOT_CONVERT_INT));
			return -1;
		}
	} else {
		if (old_type != BS_TYPE_STRING) {
			bs_error(MSG_(BS_M_CANNOT_CONVERT_STR));
			return -1;
		}
	}
	return new_type;
}

/*  Pop value and address from the stack, and store the value  */
/*  If the type is different, coersion may take place before assignment  */
int
bs_generate_store(int store_type, int value_type)
{
	if (bs_generate_coerce(store_type % BS_TYPE_LOCAL, value_type) < 0)
		return -1;
	switch (store_type) {
		case BS_TYPE_INTEGER: bs_code0(C_STORE_INT); break;
		case BS_TYPE_FLOAT:   bs_code0(C_STORE_FLT); break;
		case BS_TYPE_STRING:  bs_code0(C_STORE_STR); break;
		case BS_TYPE_INTEGER + BS_TYPE_LOCAL: bs_code0(C_FSTORE_INT); break;
		case BS_TYPE_FLOAT + BS_TYPE_LOCAL:   bs_code0(C_FSTORE_FLT); break;
		case BS_TYPE_STRING + BS_TYPE_LOCAL:  bs_code0(C_FSTORE_STR); break;
		default:
			bs_error(MSG_(BS_M_UNKNOWN_TYPE_STORE));
			return -1;
	}
	return store_type % BS_TYPE_LOCAL;
}

/*  Pop address from the stack, and push the content to the stack  */
int
bs_generate_load(int value_type)
{
	switch (value_type) {
		case BS_TYPE_INTEGER: bs_code0(C_LOAD_INT); break;
		case BS_TYPE_FLOAT:   bs_code0(C_LOAD_FLT); break;
		case BS_TYPE_STRING:  bs_code0(C_LOAD_STR); break;
		case BS_TYPE_INTEGER + BS_TYPE_LOCAL: bs_code0(C_FLOAD_INT); break;
		case BS_TYPE_FLOAT + BS_TYPE_LOCAL:   bs_code0(C_FLOAD_FLT); break;
		case BS_TYPE_STRING + BS_TYPE_LOCAL:  bs_code0(C_FLOAD_STR); break;
		default:
			bs_error(MSG_(BS_M_UNKNOWN_TYPE_LOAD));
			return -1;
	}
	return value_type % BS_TYPE_LOCAL;
}

#if 0
#pragma mark ====== Dimensions ======
#endif

/*  Invoke error if used inside IF, FOR, DO  */
int
bs_check_outside_control_statement(const char *msg)
{
	if (gParserInfo.ifaddr != kInvalidProgPos) {
		bs_error(MSG_(BS_M_NOT_INSIDE_IF), msg);
		return -1;
	}
	if (gParserInfo.lp != NULL) {
		bs_error(MSG_(BS_M_NOT_INSIDE_LOOP), msg);
		return -1;
	}
	return 0;
}

/*  Generate C_DEF_DIM_XXX. */
int
bs_generate_def_dim(int nargs, int dimidx)
{
	u_int8_t def_dim;
	DimInfo *dp = (DimInfo *)gDimInfoBasePtr + dimidx;
	bs_code1(C_PUSH_INT, nargs);
	switch (dp->code.type) {
		case BS_TYPE_INTEGER: def_dim = C_DEF_DIM_INT; break;
		case BS_TYPE_FLOAT:   def_dim = C_DEF_DIM_FLT; break;
		case BS_TYPE_STRING:  def_dim = C_DEF_DIM_STR; break;
		default:
			bs_error(MSG_(BS_M_ILLEGAL_DIM), dp->code.type);
			return -1;
	}
	bs_code1(def_dim, dp->offset);
	dp->dim = nargs;
	return 0;
}

int
bs_generate_get_dim(int nargs, int dimidx)
{
	u_int8_t get_dim;
	DimInfo *dp = (DimInfo *)gDimInfoBasePtr + dimidx;
	if (nargs != dp->dim) {
		bs_error(MSG_(BS_M_DIM_NO_MATCH), dp->dim,
				 (dp->dim == 1 ? MSG_(BS_M_INDEX) : MSG_(BS_M_INDICES)));
		return -1;
	}
	bs_code1(C_PUSH_INT, nargs);
	switch (dp->code.type) {
		case BS_TYPE_INTEGER: get_dim = C_GET_DIM_INT; break;
		case BS_TYPE_FLOAT:   get_dim = C_GET_DIM_FLT; break;
		case BS_TYPE_STRING:  get_dim = C_GET_DIM_STR; break;
		default:
			bs_error(MSG_(BS_M_ILLEGAL_DIM), dp->code.type);
			return -1;
	}
	bs_code1(get_dim, dp->offset);
	return dp->code.type;
}

#if 0
#pragma mark ====== FOR/DO statements ======
#endif

int
bs_start_loop(int type)
{
	int old_loopidx;  /*  Return value: 'previous' loop info index  */

	/*  Allocate a new loop info  */
	if (gLoopInfoTop >= gLoopInfoEnd - gLoopInfoBase - sizeof(LoopInfo)) {
		if (bs_realloc_block(MEM_LOOP_INFO, gLoopInfoEnd - gLoopInfoBase + sizeof(LoopInfo) * 32) < 0) {
			bs_error(MSG_(BS_M_TOO_MANY_LOOPS));
			return -1;
		}
	}

	if (gParserInfo.lp == NULL)
		old_loopidx = 0;
	else old_loopidx = (gParserInfo.lp - (LoopInfo *)gLoopInfoBasePtr) + 1;

	gParserInfo.lp = (LoopInfo *)(gLoopInfoBasePtr + gLoopInfoTop);
	gLoopInfoTop += sizeof(LoopInfo);
	memset(gParserInfo.lp, 0, sizeof(LoopInfo));
	
	/*  Initialize  */
	gParserInfo.lp->type = type;
	gParserInfo.lp->flags = BS_LOOPFLAG_ALLMASK;
	gParserInfo.lp->saddr = kInvalidProgPos;
	gParserInfo.lp->eaddr = kInvalidProgPos;
	gParserInfo.lp->caddr = kInvalidProgPos;
	
	if (type == BS_LOOPTYPE_FOR) {
		/*  Record the position of control variables  */
		/*  Allocation will be done later (when the type of the control variables are determined) */
		gParserInfo.lp->coff = gParserInfo.fp->ltop;
	} else if (type == BS_LOOPTYPE_DO) {
		/*  Start and continue address is the top of the statement  */
		gParserInfo.lp->saddr = gVMCodeTop;
		gParserInfo.lp->caddr = gVMCodeTop;
		gParserInfo.lp->flags &= ~(BS_LOOPFLAG_SMASK | BS_LOOPFLAG_CMASK);
	}

	return old_loopidx;
}

int
bs_complete_for_header(int var_type, int stype)
{
	u_int8_t code;

	/*  Store the STEP value  */
	if (bs_generate_store(var_type | BS_TYPE_LOCAL, stype) < 0)
		return -1;

	/*  Allocate the local variable area if necessary  */
	if (var_type % BS_TYPE_LOCAL == BS_TYPE_INTEGER)
		gParserInfo.fp->ltop = gParserInfo.lp->coff + sizeof(Off_t) + sizeof(Int) * 2;
	else
		gParserInfo.fp->ltop = gParserInfo.lp->coff + sizeof(Off_t) + sizeof(Float) * 2;
	if (gParserInfo.fp->ltop > gParserInfo.fp->lsize) {
		gParserInfo.fp->lsize = gParserInfo.fp->ltop;
		if (gParserInfo.fp->lsize >= BS_LOCAL_VAR_LIMIT) {
			bs_error(MSG_(BS_M_TOO_MANY_LOCALS));
			return -1;
		}
	}
	
	/*  Set the variable types  */
	if (var_type % BS_TYPE_LOCAL == BS_TYPE_FLOAT)
		gParserInfo.lp->flags |= BS_LOOPFLAG_ISFLOAT;
	if (var_type & BS_TYPE_LOCAL)
		gParserInfo.lp->flags |= BS_LOOPFLAG_ISLOCAL;
	
	/*  Loop end test  */
	bs_code1(C_PUSH_FREF, gParserInfo.lp->coff);
	bs_code0(C_FLOAD_REF);   /*  Load the lvalue reference  */
	switch (gParserInfo.lp->flags & (BS_LOOPFLAG_ISFLOAT | BS_LOOPFLAG_ISLOCAL)) {
		case 0:
			code = C_FORTEST_INT;
			break;
		case BS_LOOPFLAG_ISFLOAT:
			code = C_FORTEST_FLT;
			break;
		case BS_LOOPFLAG_ISLOCAL:
			code = C_FORTEST_INTL;
			break;
		default:
			code = C_FORTEST_FLTL;
			break;
	}
	bs_code1(code, gParserInfo.lp->coff);  /*  Loop end test  */
	gParserInfo.lp->eaddr = bs_code2(C_BNZ, kInvalidProgPos);

	/*  The present position is loop start  */
	gParserInfo.lp->saddr = gVMCodeTop;
	gParserInfo.lp->flags &= ~BS_LOOPFLAG_SMASK;
		
	return 0;
}

/*  Store the address to the positions pointed by the link  */
void
bs_resolve_links(Off_t *link, Off_t addr)
{
	while (*link != kInvalidProgPos) {
		Off_t new_link;
		bs_load_progpos(*link, &new_link);
		bs_store_progpos(*link, addr);
		*link = new_link;
	}
	*link = addr;
}

int
bs_generate_break_statement(void)
{
	Off_t link;
	if (gParserInfo.lp == NULL) {
		bs_error(MSG_(BS_M_BREAK_OUTSIDE_LOOP));
		return -1;
	}
	link = bs_code2(C_BRA, gParserInfo.lp->eaddr);
	if ((gParserInfo.lp->flags & BS_LOOPFLAG_EMASK) != 0)
		gParserInfo.lp->eaddr = link;
	return 0;
}

int
bs_generate_continue_statement(void)
{
	Off_t link;
	if (gParserInfo.lp == NULL) {
		bs_error(MSG_(BS_M_CONTINUE_OUTSIDE_LOOP));
		return -1;
	}
	link = bs_code2(C_BRA, gParserInfo.lp->caddr);
	if ((gParserInfo.lp->flags & BS_LOOPFLAG_CMASK) != 0)
		gParserInfo.lp->caddr = link;
	return 0;
}

/*  Follow the links in the loop body and fix the jump address  */
int
bs_finalize_loop(int save_loopidx)
{
	u_int8_t code;
	
	/*  Resolve CONTINUE links  */
	if (gParserInfo.lp->flags & BS_LOOPFLAG_CMASK) {
		bs_resolve_links(&gParserInfo.lp->caddr, gVMCodeTop);
		gParserInfo.lp->flags &= ~BS_LOOPFLAG_CMASK;
	}
	
	/*  FOR statement: generate 'NEXT' instructions  */
	if (gParserInfo.lp->type == BS_LOOPTYPE_FOR) {
		switch (gParserInfo.lp->flags & (BS_LOOPFLAG_ISFLOAT | BS_LOOPFLAG_ISLOCAL)) {
			case 0:
				code = C_NEXT_INT;
				break;
			case BS_LOOPFLAG_ISFLOAT:
				code = C_NEXT_FLT;
				break;
			case BS_LOOPFLAG_ISLOCAL:
				code = C_NEXT_INTL;
				break;
			default:
				code = C_NEXT_FLTL;
				break;
		}
		bs_code1(code, gParserInfo.lp->coff);
		bs_code2(C_BZ, gParserInfo.lp->saddr);
	}
	
	/*  Resolve BREAK links  */
	if (gParserInfo.lp->flags & BS_LOOPFLAG_EMASK) {
		bs_resolve_links(&gParserInfo.lp->eaddr, gVMCodeTop);
		gParserInfo.lp->flags &= ~BS_LOOPFLAG_EMASK;
	}
	
	/*  Restore loopinfo pointer  */
	if (save_loopidx == 0)
		gParserInfo.lp = NULL;
	else
		gParserInfo.lp = (LoopInfo *)gLoopInfoBasePtr + (save_loopidx - 1);
	
	return 0;
}

/*  stype: expression type + 0/0x10 (WHILE/UNTIL) + 0/0x20 (DO cond/LOOP cond)  */
int
bs_generate_do_cond(int stype)
{
	int is_until = ((stype & 0x10) != 0);
	int is_post_loop = ((stype & 0x20) != 0);
	Off_t link, addr;

	stype &= 0x0f;
	
	/*  Empty condition  */
	if (stype == BS_TYPE_NONE)
		return stype;

	if (stype == BS_TYPE_FLOAT)
		bs_code0(C_FLT_TO_INT);
	else if (stype != BS_TYPE_INTEGER) {
		bs_error(MSG_(BS_M_COND_NUMERIC));
		return -1;
	}
	
	if (is_post_loop) {
		addr = gParserInfo.lp->saddr;
	} else {
		addr = gParserInfo.lp->eaddr;
	}
	
	if (is_until == is_post_loop) {
		/*  Pre-loop WHILE or post-loop UNTIL  */
		link = bs_code2(C_BZ, addr); /* branch if false */
	} else {
		link = bs_code2(C_BNZ, addr); /* branch if true */
	}
	
	if (!is_post_loop) {
		gParserInfo.lp->eaddr = link;  /*  Link the unresolved address (end address)  */
	}
	
	return stype;
}

#if 0
#pragma mark ====== IF statements ======
#endif

int
bs_make_conditional_expr(int stype)
{
	if (stype == BS_TYPE_FLOAT) {
		/*  Compare with 0.0  */
		bs_code3(C_PUSH_FLT, 0.0);
		bs_code0(C_EQ_FLT);
	} else if (stype != BS_TYPE_INTEGER) {
		bs_error(MSG_(BS_M_COND_NUMERIC));
		return -1;
	}
	return 0;
}

/*  Follow the link in the IF statement and fix the jump address  */
/*  IF ... THEN <BZ @1> ... ELSEIF <BRA @3> @1 ... THEN <BZ @2> ... ELSE <BRA @3> @2 ... @3 */
void
bs_complete_if_statement(void)
{
	Off_t current = gVMCodeTop;  /*  @3  */
	Off_t branch_pos = current;
	while (gParserInfo.ifaddr != kInvalidProgPos) {
		Off_t link;
		bs_load_progpos(gParserInfo.ifaddr, &link);
		if (gVMCodeBasePtr[gParserInfo.ifaddr - 1] == C_BRA) {
			bs_store_progpos(gParserInfo.ifaddr, current);
			/*  Next to the BRA instruction (@2)  */
			branch_pos = gParserInfo.ifaddr + SIZEOF_PROGPOS;
		} else {
			bs_store_progpos(gParserInfo.ifaddr, branch_pos);
		}
		gParserInfo.ifaddr = link;
	}
}

#if 0
#pragma mark ====== Labels and GOTO ======
#endif

/*  Define label  */
int
bs_define_label(int labelidx)
{
	LabelInfo *lbp = ((LabelInfo *)gLabelInfoBasePtr) + labelidx;
	int link, curpos;

	if (lbp->defined) {
		bs_error(MSG_(BS_M_DUPLICATE_LABEL)); /*  Already defined  */
		return -1;
	}
	
	/*  If DATA/CDATA statement is open, then close it  */
	if (bs_close_data_statement() < 0)
		return -1;

	/*  Update the address fields of the GOTO forward reference  */
	link = lbp->caddr;
	curpos = gVMCodeTop;
	while (link != kInvalidProgPos) {
		int link_next;
		if (bs_check_goto_with_for_nesting_level(link, 0)) {
			bs_error(MSG_(BS_M_JUMP_INTO_LOOP_OR_FUNC));
			return -1;
		}
		bs_load_progpos(link, &link_next);
		bs_store_progpos(link, curpos);
		link = link_next;
	}
	lbp->caddr = curpos;
	
	/*  Update the address fields of the RESTORE forward reference  */
	link = lbp->daddr;
	curpos = gVMDataTop;
	while (link != kInvalidProgPos) {
		int link_next;
		bs_load_progpos(link, &link_next);
		bs_store_progpos(link, curpos);
		link = link_next;
	}
	lbp->daddr = curpos;
	
	lbp->defined = 1;

	return 0;
}

/*  Generate GOTO code  */
int
bs_generate_goto(int labelidx)
{
	LabelInfo *lbp = ((LabelInfo *)gLabelInfoBasePtr) + labelidx;
	Off_t link = lbp->caddr;
	if (gParserInfo.fp - (FuncInfo *)gFuncInfoBasePtr != lbp->scope) {
		bs_error(MSG_(BS_M_JUMP_INTO_OUTOF_FUNC));
		return -1;
	}
	bs_code2(C_BRA, link);
	if (!lbp->defined) {
		/*  The label entry is still undefined: link the address field  */
		lbp->caddr = gVMCodeTop - SIZEOF_PROGPOS;
	} else {
		if (bs_check_goto_with_for_nesting_level(link, 1)) {
			bs_error(MSG_(BS_M_JUMP_INTO_FORLOOP));
			return -1;
		}
	}
	return 0;
}

/*  Check whether GOTO jump enters FOR loop  */
int
bs_check_goto_with_for_nesting_level(int pos, int is_backward)
{
	LoopInfo *lp = (LoopInfo *)(gLoopInfoBasePtr + gLoopInfoTop);
	if (is_backward) {
		while (--lp >= (LoopInfo *)gLoopInfoBasePtr) {
			/*  Look for the FOR loop info in which pos is included  */
			if (lp->type == BS_LOOPTYPE_FOR && lp->saddr < pos) {
				if ((lp->flags & BS_LOOPFLAG_EMASK) != 0)
					return 0;  /*  OK (the latest loop is still open)  */
				else if (pos < lp->eaddr)
					return 1;  /*  NG (jump into already closed loop)  */
			}
		}
	} else {
		/*  pos must be in the newest 'still open' loop  */
		while (--lp >= (LoopInfo *)gLoopInfoBasePtr) {
			if (lp->type != BS_LOOPTYPE_FOR)
				continue;
			if ((lp->flags & BS_LOOPFLAG_EMASK) != 0) {
				/*  Newest 'still open' loop  */
				if (pos >= lp->saddr)
					return 0;  /*  OK  */
				else return 1;  /*  NG  */
			}
		}
	}
	return 0;  /*  Outside all FOR loops  */
}

/*  Follow the 'undefined entry' link and show the line numbers where the entry appears */
char *
bs_show_undefined_entry(Off_t entry)
{
	char s1[32];
	static char s[40];
	Off_t entries[16];
	const char *line_fmt;
	int i, j, len, lineno;
	i = 0;
	while (entry != kInvalidProgPos) {
		entries[i % 16] = entry;
		bs_load_progpos(entry, &entry);
		i++;
	}
	line_fmt = (i > 1 ? MSG_(BS_M_LINES_FMT) : MSG_(BS_M_LINE_FMT));
	len = 0;
	j = i - 16;
	s1[0] = 0;
	while (--i >= 0 && i >= j) {
		if (len >= sizeof(s1) - 5) {
			/*  Message is becoming too long  */
			strcat(s1, MSG_(BS_M_ETC));
			break;
		}
		lineno = bs_get_lineno_for_vmcodepos(entries[i % 16]);
		len += snprintf(s1 + len, sizeof(s1) - len, "%d%s", lineno, (i > 0 ? ", " : ""));
	}
	snprintf(s, sizeof(s), line_fmt, s1);
	return s;
}

int
bs_check_undefined_labels(int scope)
{
	int errcnt = 0;
	Off_t entry;
	LabelInfo *lbp = (LabelInfo *)(gLabelInfoBasePtr + gLabelInfoTop);
	while (--lbp >= (LabelInfo *)gLabelInfoBasePtr) {
		if (lbp->scope == scope && !lbp->defined) {
			/*  Undefined label  */
			entry = lbp->caddr;
			if (entry == kInvalidProgPos)
				entry = lbp->daddr;
			bs_error(MSG_(BS_M_LABEL_NOT_DEFINED),
					 (char *)gConstStringBasePtr + lbp->code.str,
					 bs_show_undefined_entry(entry));
			errcnt++;
		}
	}
	if (errcnt > 0)
		return -1;
	return 0;
}

int
bs_check_undefined_funcs(void)
{
	int errcnt = 0;
	Off_t entry;
	FuncInfo *fp = (FuncInfo *)(gFuncInfoBasePtr + gFuncInfoTop);
	while (--fp > (FuncInfo *)gFuncInfoBasePtr) { /* gFuncInfo[0] is dummy entry */
		if (fp->defined != 2) {
			/*  Undefined func  */
			entry = fp->saddr;
			bs_error(MSG_(BS_M_PROCFUNC_NOT_DEFINED),
					 fp->code.type == BS_TYPE_NONE ? MSG_(BS_M_PROC) : MSG_(BS_M_FUNC),
					 (char *)gConstStringBasePtr + fp->code.str,
					 bs_show_undefined_entry(entry));
			errcnt++;
		}
	}
	if (errcnt > 0)
		return -1;
	return 0;
}

#if 0
#pragma mark ====== Func/Sub calls ======
#endif

/*  Allocate a temporary signature in a runtime variable area  */
int
bs_start_funcall(int funcidx, int is_func)
{
	Off_t save_tempsig;
	TempSignature *tsp;
	const char *p;

	save_tempsig = gParserInfo.tempsig;

	/*  Check sub/func  */
	is_func = (is_func != 0);  /*  Normalize to 0 or 1  */
	if (funcidx >= 0) {
		FuncInfo *fp = (FuncInfo *)gFuncInfoBasePtr + funcidx;
		is_func |= (fp->code.type != 0 ? 2 : 0);  /*  0 or 2  */
		p = (char *)gConstStringBasePtr + fp->code.str;
	} else {
		const BuiltInInfo *bp = gBuiltIns + (-(funcidx + 1));
		is_func |= (bp->type != 0 ? 2 : 0);  /*  0 or 2  */
		p = bp->name;
	}
	if (is_func == 1) {
		bs_error(MSG_(BS_M_PROC_NOT_FUNC), p);
		return -1;
	} else if (is_func == 2) {
		bs_error(MSG_(BS_M_FUNC_NOT_PROC), p);
		return -1;
	}
	
	if (gTempSigTop + sizeof(TempSignature) >= gTempSigEnd - gTempSigBase) {
		if (bs_realloc_block(MEM_TEMP_SIG, gTempSigEnd - gTempSigBase + 2048) < 0) {
			bs_error(MSG_(BS_M_OUT_OF_MEMORY_ARG));
			return -1;
		}
	}
	tsp = (TempSignature *)(gTempSigBasePtr + gTempSigTop);
	memset(tsp, 0, sizeof(TempSignature));
	gParserInfo.tempsig = gTempSigTop / sizeof(TempSignature);
	gTempSigTop += sizeof(TempSignature);

	tsp->funcidx = funcidx;
	
	/*  C_INC_SP 0 C_NOP : dummy instruction for increasing stack pointer  */
	bs_code1(C_INC_SP, 0);
	bs_code0(C_NOP);
	tsp->incpos = gVMCodeTop - 2;
	
	return save_tempsig;
}

/*  Compare the argument type with the callee signature, coerce if necessary and possible,
	and build the temporary argument signature  */
/*  stype = 0, dim_index >= 0: the argument is a DIM name. This is allowed only for
    the built-in func/procs.  */
int
bs_check_argument(int stype, int dim_index)
{
	TempSignature *tsp = (TempSignature *)gTempSigBasePtr + gParserInfo.tempsig;
	int type_callee;
	const u_int8_t *sig_callee;
	const char *dimsig = NULL;
	DimInfo *dp = NULL;
	Off_t arg_offset;

	if (tsp->funcidx >= 0) {
		FuncInfo *fp = (FuncInfo *)gFuncInfoBasePtr + tsp->funcidx;
		sig_callee = gConstStringBasePtr + fp->sign;
	} else {
		const BuiltInInfo *bp = gBuiltIns + (-(tsp->funcidx + 1));
		sig_callee = (const u_int8_t *)(bp->sign);
		if (stype == 0) {
			/*  DIM argument  */
			dimsig = (const char *)bp->dimsign;
			dp = (DimInfo *)gDimInfoBasePtr + dim_index;
		}
	}
	
	/*  Check argument type  */
	type_callee = TYPE_OF_DATA(sig_callee, tsp->acc_callee);
	if (type_callee != 0) {
		if (stype != type_callee) {
			if (stype == 0 && dp != NULL) {
				int c = (tsp->acc.idx / 2 + 1) * 8 + dp->code.type;  /* DIM argument signature */
				if (dimsig == NULL || strchr(dimsig, c) == NULL)
					goto bad_dim_argument;
				bs_code1(C_PUSH_REF, dp->offset);
				bs_code0(C_LOAD_REF);
				stype = BS_TYPE_STRING; /*  Actually this is a reference  */
			} else {
				if (bs_generate_coerce(type_callee, stype) < 0) {
				/*	bs_error(MSG_(BS_M_ARG_CANNOT_COERCE), tsp->acc.idx / 2 + 1,
						 gTypeNames[stype], gTypeNames[type_callee]); */
					return -1;
				}
				stype = type_callee;
			}
		}
		NEXT_DATA(sig_callee, tsp->acc_callee);
	} else if (stype == 0) {
	bad_dim_argument:
		bs_error(MSG_(BS_M_BAD_DIM_ARG),
				 tsp->acc.idx / 2 + 1, gTypeNames[dp->code.type]);
		return -1;
	}
	
	/*  Add signature  */
	ADD_DATA_SIG(tsp->sigbuf, tsp->acc, stype);
	if (tsp->acc.idx >= (sizeof(tsp->sigbuf) - 1) * 8) {
		bs_error(MSG_(BS_M_TOO_MANY_ARGS));
		return -1;
	}
	
	/*  Generate VMCode for moving the value to the argument area  */
	arg_offset = -tsp->acc.ofs;
	if (arg_offset < -BS_LOCAL_VAR_LIMIT) {
		bs_error(MSG_(BS_M_TOO_MANY_ARGS));
		return -1;
	}
	switch (stype) {
		case BS_TYPE_INTEGER: bs_code1(C_SMOVE_INT, arg_offset); break;
		case BS_TYPE_FLOAT:   bs_code1(C_SMOVE_FLT, arg_offset); break;
		case BS_TYPE_STRING:  bs_code1(C_SMOVE_STR, arg_offset); break;
	}
	
	return 0;
}

/*  Generate func/sub call and cleanup code  */
int
bs_finalize_funcall(int save_tempsig)
{
	TempSignature *tsp = (TempSignature *)gTempSigBasePtr + gParserInfo.tempsig;
	Off_t sig_str, link;
	const u_int8_t *sig_callee;
	int ret_type, ret_size, arg_offset;
	FuncInfo *fp = NULL;
	const BuiltInInfo *bp = NULL;

	if (tsp->funcidx >= 0) {
		fp = (FuncInfo *)gFuncInfoBasePtr + tsp->funcidx;
		sig_callee = gConstStringBasePtr + fp->sign;
	} else {
		bp = gBuiltIns + (-(tsp->funcidx + 1));
		sig_callee = (const u_int8_t *)bp->sign;
	}
	
	/*  Store the SP offset  */
	bs_store_operand(tsp->incpos, tsp->acc.ofs);
	
	/*  Allocate the argument signature string and push to the stack  */
	sig_str = bs_new_literal_string((char *)tsp->sigbuf, -1);
	bs_code4(C_PUSH_STRL, sig_str);
	
	/*  Ensure all requested arguments are given  */
	if (TYPE_OF_DATA(sig_callee, tsp->acc_callee) != 0) {
		/*  There is an argument that is not given  */
		bs_error(MSG_(BS_M_MISSING_ARG), tsp->acc_callee.idx / 2 + 1);
		return -1;
	}
	
	/*  If signature of FUNC/PROC is not given, then set it  */
	if (tsp->funcidx >= 0 && fp->defined == 0) {
		fp->sign = sig_str & ~kMSBOff;
		fp->defined = 1;
	}
	
	/*  Jump to subroutine  */
	if (tsp->funcidx >= 0) {
		link = bs_code2(C_BSR, fp->saddr);
		if (fp->defined != 2)
			fp->saddr = link;  /*  Create link  */
		ret_type = fp->code.type;
	} else {
		bs_code1(C_BUILTIN_SUB, -(tsp->funcidx + 1));
		ret_type = bp->type;
	}
	ret_size = gTypeSize[ret_type];
	arg_offset = -(sizeof(Off_t) + tsp->acc.ofs);

	/*  Discard the arguments: the string value should be released  */
	while (LAST_DATA(tsp->sigbuf, tsp->acc)) {
		if (TYPE_OF_DATA(tsp->sigbuf, tsp->acc) == BS_TYPE_STRING) {
			bs_code1(C_RELSTR_SP, -(ret_size + sizeof(Off_t) * 2 + tsp->acc.ofs));
		}
	}
	
	/*  Move the return value to the stack top after discarding the arguments  */
	switch (ret_type) {
		case BS_TYPE_INTEGER: bs_code1(C_SMOVE_INT, arg_offset); break;
		case BS_TYPE_FLOAT:   bs_code1(C_SMOVE_FLT, arg_offset); break;
		case BS_TYPE_STRING:  bs_code1(C_SMOVE_STR, arg_offset); break;
	}
	
	/*  Adjust the stack pointer (size of all arguments - size of return value)  */
	bs_code1(C_INC_SP, arg_offset + ret_size);
	
	/*  Release temporary signature  */
	gParserInfo.tempsig = save_tempsig;
	gTempSigTop -= sizeof(TempSignature);
	
	return ret_type;
}

int
bs_start_funcdef(int funcidx, int is_func)
{
	TempSignature *tsp;
	
	if (gParserInfo.fp != (FuncInfo *)gFuncInfoBasePtr) {
		bs_error(MSG_(BS_M_PROCFUNC_DEF_NESTED));
		return -1;
	}
	if (bs_check_outside_control_statement(MSG_(BS_M_PROCFUNC_DEF)) < 0)
		return -1;
	
	gParserInfo.fp = (FuncInfo *)gFuncInfoBasePtr + funcidx;

	/*  Allocate temporary signature  */
	if (gTempSigTop + sizeof(TempSignature) >= gTempSigEnd - gTempSigBase) {
		if (bs_realloc_block(MEM_TEMP_SIG, gTempSigEnd - gTempSigBase + 2048) < 0) {
			bs_error(MSG_(BS_M_OUT_OF_MEMORY_ARG));
			return -1;
		}
	}
	
	/*  gParserInfo.tempsig must be 0; we don't check it here  */
	tsp = (TempSignature *)(gTempSigBasePtr + gTempSigTop);
	memset(tsp, 0, sizeof(TempSignature));
	gParserInfo.tempsig = gTempSigTop / sizeof(TempSignature);
	gTempSigTop += sizeof(TempSignature);
	
	tsp->funcidx = funcidx;
	
	gParserInfo.local_var = 1; /* Start definition of formal arguments */
	
	return 0;
}

/*  Define a formal argument  */
int
bs_check_formal_argument(int varidx)
{
	TempSignature *tsp = (TempSignature *)gTempSigBasePtr + gParserInfo.tempsig;
	VarInfo *vp = (VarInfo *)gVarInfoBasePtr + varidx;
	int type_impl;
	u_int8_t *sig_impl; /* Signature defined in implicit declaration */

	if (gParserInfo.fp->defined == 1) {
		int fp_type = gParserInfo.fp->code.type;
		sig_impl = gConstStringBasePtr + gParserInfo.fp->sign;
		type_impl = TYPE_OF_DATA(sig_impl, tsp->acc_callee);
		if (type_impl == 0) {
			bs_error(MSG_(BS_M_FORMAL_ARGS_MISMATCH_NUM),
					 (fp_type == 0 ? MSG_(BS_M_PROC) : MSG_(BS_M_FUNC)), tsp->acc_callee.idx / 2);
			return -1;
		}
		if (type_impl != vp->code.type) {
			bs_error(MSG_(BS_M_FORMAL_ARG_MISMATCH_TYPE),
					 (fp_type == 0 ? MSG_(BS_M_PROC) : MSG_(BS_M_FUNC)),
					 tsp->acc_callee.idx / 2, gTypeNames[type_impl],
					 gTypeNames[vp->code.type]);
			return -1;
		}
	}
	
	/*  Add signature  */
	ADD_DATA_SIG(tsp->sigbuf, tsp->acc, vp->code.type);
	if (tsp->acc.idx >= (sizeof(tsp->sigbuf) - 1) * 8) {
		bs_error(MSG_(BS_M_TOO_MANY_FORMAL_ARGS));
		return -1;
	}
	
	/*  Offset from the Frame pointer  */
	/*  sizeof(Off_t) * 3 is for (1) signature string, (2) return address, (3) old frame pointer  */
	vp->offset = -(sizeof(Off_t) * 3 + tsp->acc.ofs);

	return varidx;
}

/*  Close formal argument list and start definition of func/proc body  */
int
bs_close_formal_argument_list(void)
{
	TempSignature *tsp = (TempSignature *)gTempSigBasePtr + gParserInfo.tempsig;

	gParserInfo.local_var = 0; /* End definition of formal arguments */
	
	if (gParserInfo.fp->defined == 0) {
		/*  Allocate the argument signature string and keep it in the FuncInfo  */
		Off_t sig_str = bs_new_literal_string((char *)tsp->sigbuf, -1);
		gParserInfo.fp->sign = sig_str & ~kMSBOff;
	}
	gParserInfo.fp->defined = 2;
	
	/*  Skip instruction  */
	/*  The address will be fixed during finalization  */
	bs_code2(C_BRA, kInvalidProgPos);
	
	/*  func/proc entry address  */
	/*  Resolve links for forward invocation  */
	bs_resolve_links(&(gParserInfo.fp->saddr), gVMCodeTop);
	
	/*  The first instruction is C_LINK, which allocates local variable area in the stack  */
	/*  Up to 2 bytes (max 8191) is allowed, which is the maximum of the local variable area  */
	/*  The present version allows only simple local variables (i.e. no dimensions),
	    so this should be sufficient */
	bs_code1(C_LINK, 0);
	bs_code0(C_NOP);	
	
	/*  Release temporary signature  */
	gTempSigTop -= sizeof(TempSignature);
	gParserInfo.tempsig = 0;
		
	return 0;
}

/*  Finalize FUNC/PROC definition  */
int
bs_finalize_funcdef(int is_func)
{
	int is_func_def;

	is_func = (is_func != 0);  /*  Normalize to 1/0  */
	is_func_def = (gParserInfo.fp->code.type != BS_TYPE_NONE);
	if (is_func != is_func_def) {
		bs_error(MSG_(BS_M_FUNCDEF_BAD_END),
				 (is_func_def ? MSG_(BS_M_FUNC) : MSG_(BS_M_PROC)),
				 (is_func ? "ENDFUNC" : "ENDPROC"));
		return -1;
	}
	
	if (is_func && gParserInfo.fp->eaddr == kInvalidProgPos) {
		bs_error("FUNC definition does not contain RETURN statement");
		return -1;
	}
	bs_resolve_links(&(gParserInfo.fp->eaddr), gVMCodeTop);
	switch (gParserInfo.fp->code.type) {
		case BS_TYPE_NONE: bs_code0(C_RETURN); break;
		case BS_TYPE_INTEGER: bs_code0(C_RET_INT); break;
		case BS_TYPE_FLOAT:   bs_code0(C_RET_FLT); break;
		case BS_TYPE_STRING:  bs_code0(C_RET_STR); break;
	}
	gParserInfo.fp->eaddr++;
	
	/*  Complete the LINK statement by storing the local variable size  */
	bs_store_operand(gParserInfo.fp->saddr + 1, gParserInfo.fp->lsize);
	
	/*  Complete the BRA statement by storing the end address  */
	bs_store_progpos(gParserInfo.fp->saddr - SIZEOF_PROGPOS, gParserInfo.fp->eaddr);
	
	/*  Return to the global scope  */
	gParserInfo.fp = (FuncInfo *)gFuncInfoBasePtr;
	
	return 0;
}

int
bs_generate_return(int stype)
{
	if (gParserInfo.fp == (FuncInfo *)gFuncInfoBasePtr) {
		bs_error(MSG_(BS_M_RETURN_OUTSIDE_FUNC));
		return -1;
	}
	
	if (gParserInfo.fp->code.type == 0) {
		if (stype != 0) {
			bs_error(MSG_(BS_M_RETURN_NO_VALUE));
			return -1;
		}
	} else {
		if (stype == 0) {
			bs_error(MSG_(BS_M_RETURN_MISSING_VALUE));
			return -1;
		}
		if (bs_generate_coerce(gParserInfo.fp->code.type, stype) < 0)
			return -1;
	}
	
	gParserInfo.fp->eaddr = bs_code2(C_BRA, gParserInfo.fp->eaddr);  /*  Create link  */
	return 0;
}

int
bs_start_local_statement(void)
{
	if (gParserInfo.fp == (FuncInfo *)gFuncInfoBasePtr) {
		bs_error(MSG_(BS_M_LOCAL_OUTSIDE_FUNC));
		return -1;
	}
	if (bs_check_outside_control_statement("LOCAL statement") < 0)
		return -1;

	gParserInfo.local_var = 2; /* Start definition of local variables */
	
	return 0;
}

/*  Define a local variable  */
int
bs_check_local_var(int varidx)
{
	VarInfo *vp = (VarInfo *)gVarInfoBasePtr + varidx;
	
	/*  Offset from the Frame pointer  */
	vp->offset = gParserInfo.fp->lsize;
	gParserInfo.fp->lsize += gTypeSize[vp->code.type];
	if (gParserInfo.fp->lsize >= BS_LOCAL_VAR_LIMIT) {
		bs_error(MSG_(BS_M_TOO_MANY_LOCAL_VARS));
		return -1;
	}
	
	/*  ltop is set to lsize; this means that the local memory for implicit variables 
	 in FOR loops will not be used in the following LOCAL statements (and in FOR loops
	 afterwards) */
	gParserInfo.fp->ltop = gParserInfo.fp->lsize;
	
	return varidx;
}

int
bs_finalize_local_statement(void)
{
	gParserInfo.local_var = 0; /* End definition of local variables */
	return 0;
}


#if 0
#pragma mark ====== DATA/CDATA ======
#endif

int
bs_start_data_statement(u_int8_t code)
{
	memset(gTempSigBasePtr, 0, sizeof(TempSignature));
	gParserInfo.dataaddr = gVMDataTop;
	bs_datagen_byte(code);
	bs_datagen_ofs(0);
	gVMDataTop = gParserInfo.dataaddr + 4;  /*  Header is always 4 bytes  */
	return 0;
}

int
bs_generate_data(TokenValue *tp)
{
	TempSignature *tsp = (TempSignature *)gTempSigBasePtr;

	/*  Is data statement closed?  */
	if (gParserInfo.dataaddr == kInvalidProgPos) {
		if (bs_start_data_statement(C_DATA) < 0)
			return -1;
	}

	/*  Generate data  */
	switch (tp->type) {
		case BS_TYPE_INTEGER: bs_datagen_integer(tp->u.iv); break;
		case BS_TYPE_FLOAT:   bs_datagen_float(tp->u.fv); break;
		case BS_TYPE_STRING:
			bs_datagen_str(tp->u.sv);
#if BS_OFF_T_IS_32BIT
			bs_datagen_byte(0);  /*  Dummy one byte to make it 4 bytes  */
#endif
			break;
	}
	
	/*  Add signature  */
	ADD_DATA_SIG(tsp->sigbuf, tsp->acc, tp->type);
	
	/*  Is signature too long?  */
	if (tsp->acc.idx >= (sizeof(tsp->sigbuf) - 1) * 8) {
		/*  This row of data is closed at this time  */
		if (bs_close_data_statement() < 0)
			return -1;
	}
	
	return 0;
}

int
bs_generate_cdata(TokenValue *tp)
{
	TempSignature *tsp = (TempSignature *)gTempSigBasePtr;
	
	/*  Is data statement closed?  */
	if (gParserInfo.dataaddr == kInvalidProgPos) {
		if (bs_start_data_statement(C_CDATA) < 0)
			return -1;
	}
	
	/*  Generate data  */
	if (tp->type != BS_TYPE_INTEGER) {
		bs_error(MSG_(BS_M_CDATA_ONLY_INTEGER));
		return -1;
	}
	
	bs_datagen_byte(tp->u.iv);
	
	/*  tsp->acc.ofs is used for counting the number of data  */
	if (++(tsp->acc.ofs) >= 8000) {
		if (bs_close_data_statement() < 0)
			return -1;
	}
	
	return 0;
}

int
bs_close_data_statement(void)
{
	TempSignature *tsp = (TempSignature *)gTempSigBasePtr;

	if (gParserInfo.dataaddr != kInvalidProgPos) {
		Off_t save_datatop = gVMDataTop;
		Off_t str = bs_new_literal_string((char *)tsp->sigbuf, -1);
		gVMDataTop = gParserInfo.dataaddr;
		switch (gVMDataBasePtr[gVMDataTop++]) {
			case C_DATA:
				/*  Operand of C_DATA (offset to the signature string) is completed  */
				bs_datagen_ofs(str & ~kMSBOff);
				break;
			case C_CDATA:
				bs_datagen_ofs(tsp->acc.ofs);
				break;
		}
		gVMDataTop = save_datatop;
		gParserInfo.dataaddr = kInvalidProgPos;
	}
	return 0;
}

/*  Generate RESTORE  */
int
bs_generate_restore(int labelidx)
{
	LabelInfo *lbp = ((LabelInfo *)gLabelInfoBasePtr) + labelidx;
	Off_t link = lbp->daddr;
	bs_code2(C_RESTORE, link);
	if (!lbp->defined) {
		/*  The label entry is still undefined: link the address field  */
		lbp->daddr = gVMCodeTop - SIZEOF_PROGPOS;
	}
	return 0;
}

/*  Generate READ_XXX and STORE_XXX (or FSTORE_XXX)  */
int
bs_generate_read_and_store(int store_type)
{
	switch (store_type) {
		case BS_TYPE_INTEGER: bs_code0(C_READ_INT); bs_code0(C_STORE_INT); break;
		case BS_TYPE_FLOAT:   bs_code0(C_READ_FLT); bs_code0(C_STORE_FLT); break;
		case BS_TYPE_STRING:  bs_code0(C_READ_STR); bs_code0(C_STORE_STR); break;
		case BS_TYPE_INTEGER + BS_TYPE_LOCAL: bs_code0(C_READ_INT); bs_code0(C_FSTORE_INT); break;
		case BS_TYPE_FLOAT + BS_TYPE_LOCAL:   bs_code0(C_READ_FLT); bs_code0(C_FSTORE_FLT); break;
		case BS_TYPE_STRING + BS_TYPE_LOCAL:  bs_code0(C_READ_STR); bs_code0(C_FSTORE_STR); break;
		default:
			bs_error(MSG_(BS_M_UNKNOWN_TYPE_READ));
			return -1;
	}
	return store_type % BS_TYPE_LOCAL;
}
