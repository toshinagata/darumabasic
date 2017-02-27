/*
 *  builtin.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 2015/12/31.
 *  Copyright 2015 Toshi Nagata.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "daruma.h"
#include "y.tab.h"

#include "screen.h"
#include "graph.h"

#include "builtin_argtypes.h"

#if 0
#pragma mark ====== Mathematical Functions ======
#endif

int
bs_builtin_sin(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_float_value(sin(dval));
	return 0;
}

int
bs_builtin_cos(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_float_value(cos(dval));
	return 0;
}

int
bs_builtin_tan(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_float_value(tan(dval));
	return 0;
}

int
bs_builtin_exp(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_float_value(exp(dval));
	return 0;
}

int
bs_builtin_log(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_float_value(log(dval));
	return 0;
}

int
bs_builtin_abs(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_float_value(fabs(dval));
	return 0;
}

int
bs_builtin_intabs(void)
{
	Int ival;
	bs_get_next_int_arg(&ival);
	if (ival < 0)
		ival = ival;
	bs_push_int_value(ival);
	return 0;
}

int
bs_builtin_sqr(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_float_value(sqrt(dval));
	return 0;
}

int
bs_builtin_rnd(void)
{
	bs_push_float_value(rand() / ((double)(RAND_MAX) + 1));
	return 0;
}

int
bs_builtin_int(void)
{
	Float dval;
	bs_get_next_float_arg(&dval);
	bs_push_int_value(floor(dval));
	return 0;
}

int
bs_builtin_float(void)
{
	Int ival;
	bs_get_next_int_arg(&ival);
	bs_push_float_value((Float)ival);
	return 0;
}

int
bs_builtin_mod(void)
{
	Int x, y, r;
	bs_get_next_int_arg(&x);
	bs_get_next_int_arg(&y);
	if ((x > 0 && y < 0) || (x < 0 && y > 0))
		r = x % y + y;
	else r = x % y;
	bs_push_int_value(r);
	return 0;
}

int
bs_builtin_remainder(void)
{
	Int x, y, r;
	bs_get_next_int_arg(&x);
	bs_get_next_int_arg(&y);
	r = x % y;
	bs_push_int_value(r);
	return 0;
}

#if 0
#pragma mark ====== String Functions ======
#endif

int
bs_builtin_left(void)
{
	Off_t str;
	const char *p;
	Int n, len;
	bs_get_next_str_arg(&str);
	bs_get_next_int_arg(&n);
	p = bs_get_string_ptr(str);
	if (n <= 0) {
		str = bs_new_literal_string("", 0);
	} else {
		len = strlen(p);
		if (n < len)
			str = bs_new_runtime_string(p, n);
	}
	bs_push_str_value(str);
	return 0;
}

int
bs_builtin_right(void)
{
	Off_t str;
	const char *p;
	Int n, len;
	bs_get_next_str_arg(&str);
	bs_get_next_int_arg(&n);
	p = bs_get_string_ptr(str);
	if (n <= 0) {
		str = bs_new_literal_string("", 0);
	} else {
		len = strlen(p);
		if (n < len)
			str = bs_new_runtime_string(p + (len - n), n);
	}
	bs_push_str_value(str);
	return 0;
}

int
bs_builtin_mid(void)
{
	Off_t str;
	const char *p;
	Int n1, n2, len;
	bs_get_next_str_arg(&str);
	bs_get_next_int_arg(&n1);
	bs_get_next_int_arg(&n2);
	p = bs_get_string_ptr(str);
	len = strlen(p);
	n1--;
	if (n2 <= 0 || n1 < 0 || n1 >= len) {
		str = bs_new_literal_string("", 0);
	} else {
		if (n1 + n2 > len)
			n2 = len - n1;
		str = bs_new_runtime_string(p + n1, n2);
	}
	bs_push_str_value(str);
	return 0;
}

int
bs_builtin_chr(void)
{
	Int n;
	Off_t str;
	bs_get_next_int_arg(&n);
	if (n < 0 || n >= 256)
		str = bs_new_literal_string("", 0);
	else {
		char s[2];
		s[0] = n;
		s[1] = 0;
		str = bs_new_runtime_string(s, 1);
	}
	bs_push_str_value(str);
	return 0;
}

int
bs_builtin_asc(void)
{
	Off_t str;
	const char *p;
	Int n;
	bs_get_next_str_arg(&str);
	if (bs_get_next_int_arg(&n))
		n--;
	else n = 0;
	p = bs_get_string_ptr(str);
	if (n <= 0)
		n = (Int)(u_int8_t)*p;
	else if (n >= strlen(p))
		n = 0;
	else n = (Int)(u_int8_t)p[n];
	bs_push_int_value(n);
	return 0;
}

int
bs_builtin_str(void)
{
	Off_t str;
	Float dval;
	char s[32];
	bs_get_next_float_arg(&dval);
	snprintf(s, sizeof(s), "%.16g", dval);
	str = bs_new_runtime_string(s, -1);
	bs_push_str_value(str);
	return 0;
}

int
bs_builtin_intval(void)
{
	Off_t str;
	const char *p;
	Int ival;
	bs_get_next_str_arg(&str);
	p = bs_get_string_ptr(str);
	ival = strtol(p, NULL, 0);
	bs_push_int_value(ival);
	return 0;
}

int
bs_builtin_val(void)
{
	Off_t str;
	const char *p;
	Float dval;
	bs_get_next_str_arg(&str);
	p = bs_get_string_ptr(str);
	dval = strtod(p, NULL);
	bs_push_float_value(dval);
	return 0;
}

#if 0
#pragma mark ====== System Functions ======
#endif

int
bs_builtin_uptime(void)
{
	u_int64_t u = bs_uptime(0);
	bs_push_float_value(u / 1000000.0);
	return 0;
	
}

int
bs_builtin_testsub(void)
{
	extern void bs_dump_screen(void);
	Int x, y;
	bs_get_next_int_arg(&x);
	bs_get_next_int_arg(&y);
	debug_printf("%d %d\n", x, y);
	bs_dump_screen();
	bs_push_int_value(0);
	return 0;
}

const BuiltInInfo gBuiltIns[] = {
	{ BS_TYPE_FLOAT,   "SIN",    bs_builtin_sin, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_FLOAT,   "COS",    bs_builtin_cos, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_FLOAT,   "TAN",    bs_builtin_tan, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_FLOAT,   "EXP",    bs_builtin_exp, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_FLOAT,   "LOG",    bs_builtin_log, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_FLOAT,   "ABS",    bs_builtin_abs, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_INTEGER, "INTABS", bs_builtin_intabs, (SIGTYPE)(BS_ARG_IXXX), 0},
	{ BS_TYPE_FLOAT,   "SQR",    bs_builtin_sqr, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_FLOAT,   "RND",    bs_builtin_rnd, (SIGTYPE)(BS_ARG_XXXX), 0},
	{ BS_TYPE_INTEGER, "INT",    bs_builtin_int, (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_INTEGER, "MOD",    bs_builtin_mod, (SIGTYPE)(BS_ARG_IIXX), 0},
	{ BS_TYPE_INTEGER, "REMAINDER", bs_builtin_remainder, (SIGTYPE)(BS_ARG_IIXX), 0},
	{ BS_TYPE_INTEGER, "TESTSUB",   bs_builtin_testsub,   (SIGTYPE)(BS_ARG_IIXX), 0},
	{ BS_TYPE_FLOAT,   "FLOAT",  bs_builtin_float,  (SIGTYPE)(BS_ARG_IXXX), 0},
	{ BS_TYPE_STRING,  "LEFT$",  bs_builtin_left,   (SIGTYPE)(BS_ARG_SIXX), 0},
	{ BS_TYPE_STRING,  "RIGHT$", bs_builtin_right,  (SIGTYPE)(BS_ARG_SIXX), 0},
	{ BS_TYPE_STRING,  "MID$",   bs_builtin_mid,    (SIGTYPE)(BS_ARG_SIIX), 0},
	{ BS_TYPE_STRING,  "CHR$",   bs_builtin_chr,    (SIGTYPE)(BS_ARG_IXXX), 0},
	{ BS_TYPE_INTEGER, "ASC",    bs_builtin_asc,    (SIGTYPE)(BS_ARG_SXXX), 0},
	{ BS_TYPE_STRING,  "STR$",   bs_builtin_str,    (SIGTYPE)(BS_ARG_FXXX), 0},
	{ BS_TYPE_INTEGER, "INTVAL", bs_builtin_intval, (SIGTYPE)(BS_ARG_SXXX), 0},
	{ BS_TYPE_FLOAT,   "VAL",    bs_builtin_val,    (SIGTYPE)(BS_ARG_SXXX), 0},
	{ BS_TYPE_FLOAT,   "UPTIME", bs_builtin_uptime, (SIGTYPE)(BS_ARG_XXXX), 0},
	
	/*  edit.c  */
	{ BS_TYPE_NONE,    "LOAD",   bs_builtin_load, (SIGTYPE)(BS_ARG_SXXX), 0},
	{ BS_TYPE_NONE,    "SAVE",   bs_builtin_save, (SIGTYPE)(BS_ARG_SXXX), 0},
	{ BS_TYPE_NONE,    "LIST",   bs_builtin_list, (SIGTYPE)(BS_ARG_XXXX), 0},
	
	/*  screen.c  */
	{ BS_TYPE_NONE,    "LOCATE", bs_builtin_locate, (SIGTYPE)(BS_ARG_IIXX), 0},
	{ BS_TYPE_NONE,    "CLS",    bs_builtin_cls, (SIGTYPE)(BS_ARG_XXXX), 0},
	{ BS_TYPE_NONE,    "CLEARLINE", bs_builtin_clearline, (SIGTYPE)(BS_ARG_XXXX), 0},
	{ BS_TYPE_NONE,    "COLOR",  bs_builtin_color, (SIGTYPE)(BS_ARG_IXXX), 0},
	{ BS_TYPE_NONE,    "TCOLOR", bs_builtin_tcolor, (SIGTYPE)(BS_ARG_IXXX), 0},
	{ BS_TYPE_INTEGER, "INKEY",  bs_builtin_inkey, (SIGTYPE)(BS_ARG_XXXX), 0},
	{ BS_TYPE_INTEGER, "SCREENSIZE", bs_builtin_screensize, (SIGTYPE)(BS_ARG_IXXX), 0},
	
	/*  graph.c  */
	{ BS_TYPE_NONE,    "GCOLOR", bs_builtin_gcolor, (SIGTYPE)(BS_ARG_IXXX), 0},	
	{ BS_TYPE_NONE,    "FILLCOLOR", bs_builtin_fillcolor, (SIGTYPE)(BS_ARG_IXXX), 0},	
	{ BS_TYPE_NONE,    "PSET",   bs_builtin_pset,   (SIGTYPE)(BS_ARG_IIXX), 0},	
	{ BS_TYPE_NONE,    "LINE",   bs_builtin_line,   (SIGTYPE)(BS_ARG_IIII), 0},	
	{ BS_TYPE_NONE,    "LINETO", bs_builtin_lineto, (SIGTYPE)(BS_ARG_IIXX), 0},	
	{ BS_TYPE_NONE,    "CIRCLE", bs_builtin_circle, (SIGTYPE)(BS_ARG_IIIX), 0},	
	{ BS_TYPE_NONE,    "ARC",    bs_builtin_arc,    (SIGTYPE)(BS_ARG_IIFF BS_ARG_IXXX), 0},	
	{ BS_TYPE_NONE,    "FAN",    bs_builtin_fan,    (SIGTYPE)(BS_ARG_IIFF BS_ARG_IXXX), 0},	
	{ BS_TYPE_NONE,    "POLY",   bs_builtin_poly,   (SIGTYPE)(BS_ARG_IIXX), 0},	
	{ BS_TYPE_NONE,    "BOX",    bs_builtin_box,    (SIGTYPE)(BS_ARG_IIII), 0},	
	{ BS_TYPE_NONE,    "RBOX",   bs_builtin_rbox,   (SIGTYPE)(BS_ARG_IIII BS_ARG_IXXX), 0},	
	{ BS_TYPE_NONE,    "GCLS",   bs_builtin_gcls,   (SIGTYPE)(BS_ARG_XXXX), 0},	
	{ BS_TYPE_NONE,    "GMODE",  bs_builtin_gmode,  (SIGTYPE)(BS_ARG_IXXX), 0},	
	{ BS_TYPE_NONE,    "PATDEF", bs_builtin_patdef, (SIGTYPE)(BS_ARG_IIIS), (SIGTYPE)"\041"},
	{ BS_TYPE_NONE,    "PATUNDEF", bs_builtin_patundef, (SIGTYPE)(BS_ARG_IXXX), 0},
	{ BS_TYPE_NONE,    "PATDRAW", bs_builtin_patdraw, (SIGTYPE)(BS_ARG_IIIX), 0},
	{ BS_TYPE_NONE,    "PATERASE", bs_builtin_paterase, (SIGTYPE)(BS_ARG_IIIX), 0},
	{ BS_TYPE_INTEGER, "RGB",    bs_builtin_rgb,    (SIGTYPE)(BS_ARG_FFFX), 0},	
	{ BS_TYPE_NONE,    "REDRAW", bs_builtin_redraw, (SIGTYPE)(BS_ARG_IXXX), 0},	
};

const int gNumberOfBuiltIns = (sizeof(gBuiltIns) / sizeof(gBuiltIns[0]));
