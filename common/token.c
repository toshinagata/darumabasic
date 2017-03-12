/*
 *  token.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 2015/12/07.
 *  Copyright 2015 Toshi Nagata.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "daruma.h"
#include "gencode.h"
#include "screen.h"
#include "transmessage.h"
#include "y.tab.h"

TokenValue gTokenValue;
TokenValue gZeroTokenValue;
int gLineno;

/*  Pointer for reading source text  */
static const u_int8_t *s_pos;
static FILE *s_fpin;
static int s_pushback;

static const int s_bufsize = 256;
static u_int8_t s_buf[256];
static int s_bufindex;

int
bs_start_parser_on_memory(const u_int8_t *pos, int new_run)
{
	s_pushback = -1;
	s_bufindex = -1;
	s_fpin = NULL;
	s_pos = pos;
	gParserInfo.lineno = 0;
	if (new_run)
		gVMAddressTop = 0;
	return 0;
}

void
bs_error(const char *fmt, ...)
{
	va_list arg;
	int n, c;
	char msg[512];
	
	if (fmt != NULL && fmt[0] != 0) {
		va_start(arg, fmt);
		vsnprintf(msg, sizeof msg, fmt, arg);
		va_end(arg);
		bs_tcolor(RGBFLOAT(1, 0.2, 0.2));
		bs_puts(msg);
		bs_puts("\n");
	}
	
	bs_tcolor(RGBFLOAT(1, 1, 1));
	snprintf(msg, sizeof msg, MSG_(BS_M_LINE), gParserInfo.lineno);
	bs_puts(msg);
	for (n = 0; s_buf[n] != 0; n++) {
		if (n == s_bufindex || (n == s_bufindex - 1 && s_buf[n + 1] == 0)) {
			c = s_buf[n];
			s_buf[n] = 0;
			bs_puts((char *)s_buf);
			s_buf[n] = c;
			bs_tcolor(RGBFLOAT(1, 0.2, 0.2));
			bs_puts("(???)");
			bs_tcolor(RGBFLOAT(1, 1, 1));
			bs_puts((char *)s_buf + n);
			break;
		}
	}
	bs_tcolor(RGBFLOAT(1, 1, 1));
	bs_puts("\n");
}

void
bs_runtime_error(const char *fmt, ...)
{
	va_list arg;
	int n;
	char msg[512];
	Off_t codepos;

	/*  Reset text color  */
	bs_tcolor(RGBFLOAT(1, 1, 1));
	bs_bgcolor(RGBFLOAT(0, 0, 0));
	
	codepos = bs_codepos();
	n = bs_get_lineno_for_vmcodepos(codepos);
	snprintf(msg, sizeof msg, MSG_(BS_M_RUNTIME_ERROR), n);
	
	if (fmt != NULL && fmt[0] != 0) {
		va_start(arg, fmt);
		vsnprintf(msg, sizeof msg, fmt, arg);
		va_end(arg);
		bs_puts(msg);
		bs_puts("\n");
	}
}

#if 0
#pragma mark ====== Reserved Words ======
#endif

const char gReservedWordsString[] =
"IF\0" "THEN\0" "ELSEIF\0" "ELSE\0" "ENDIF\0"
"DO\0" "WHILE\0" "UNTIL\0" "LOOP\0"
"FOR\0" "TO\0" "STEP\0" "NEXT\0"
"GOTO\0" "FUNC\0" "PROC\0" "RETURN\0" "EXIT\0"
"ENDPROC\0" "ENDFUNC\0"
"DIM\0" "PRINT\0" "INPUT\0"
"LET\0" "LOCAL\0" "CALL\0"
"BREAK\0" "CONTINUE\0"
"CDATA\0" "DATA\0" "READ\0" "RESTORE\0"
"END\0"
"OR\0" "AND\0" "XOR\0" "NOT\0"
"WAIT\0";

const Int gReservedWords[] = {
	C(BS_IF), C(BS_THEN), C(BS_ELSEIF), C(BS_ELSE), C(BS_ENDIF),
	C(BS_DO), C(BS_WHILE), C(BS_UNTIL), C(BS_LOOP),
	C(BS_FOR), C(BS_TO), C(BS_STEP), C(BS_NEXT),
	C(BS_GOTO), C(BS_FUNC), C(BS_PROC), C(BS_RETURN), C(BS_EXIT),
	C(BS_ENDPROC), C(BS_ENDFUNC),
	C(BS_DIM), C(BS_PRINT), C(BS_INPUT),
	C(BS_LET), C(BS_LOCAL), C(BS_CALL),
	C(BS_BREAK), C(BS_CONTINUE),
	C(BS_CDATA), C(BS_DATA), C(BS_READ), C(BS_RESTORE),
	C(BS_END),
	C(BS_OR), C(BS_AND), C(BS_XOR), C(BS_NOT),
    C(BS_WAIT) };

const char *gTypeNames[] = { "", "Integer", "Float", "String" };

int
bs_lookup_reserved_words(char *name, int *len)
{
	int i, k;
	const char *p = gReservedWordsString;
	for (i = 0; *p != 0; p += k + 1, i++) {
		for (k = 1; p[k] != 0; k++);
		if (strncasecmp(name, p, k) == 0 && !isalnum(name[k])) {
			if (len != NULL)
				*len = k;
			return T(gReservedWords[i]);
		}
	}
	return -1;
}

#if 0
#pragma mark ====== Built-In Commands and Functions ======
#endif

int
bs_lookup_builtin_commands(char *name)
{
	int i;
	for (i = 0; i < gNumberOfBuiltIns; i++) {
		if (strcasecmp(gBuiltIns[i].name, name) == 0)
			return i;
	}
	return -1;
}

#if 0
#pragma mark ====== Variables ======
#endif

int
bs_define_var(SymbolCode symcode, int scope)
{
	VarInfo *vp;
	if (gVarInfoTop >= gVarInfoEnd - gVarInfoBase) {
		bs_error("Too many variables");
		return -1;
	}
	vp = (VarInfo *)(gVarInfoBasePtr + gVarInfoTop);
	gVarInfoTop += sizeof(VarInfo);
	memset(vp, 0, sizeof(VarInfo));
	vp->code = symcode;
	vp->scope = scope;
	if (scope == 0) {
		/*  Global variables  */
		/*  (For local variables, vp->offset will be set later)  */
		vp->offset = gParserInfo.varreq;
		gParserInfo.varreq += gTypeSize[vp->code.type % 8];
	}
	return gVarInfoTop / sizeof(VarInfo) - 1;  /*  The index to gVarInfo  */
}

int
bs_lookup_label(SymbolCode symcode)
{
	int i = gLabelInfoTop / sizeof(LabelInfo) - 1;
	int funcidx = gParserInfo.fp - (FuncInfo *)gFuncInfoBasePtr;
	LabelInfo *lbp = (LabelInfo *)gLabelInfoBasePtr + i;
	for (; i >= 0; i--, lbp--) {
		if (EqualSymbolCode(lbp->code, symcode) && lbp->scope == funcidx)
			return i;
	}
	/*  If undefined, then implicitly define it  */
	if (gLabelInfoTop + sizeof(LabelInfo) >= gLabelInfoEnd - gLabelInfoBase) {
		if (bs_realloc_block(MEM_LABEL_INFO, gLabelInfoEnd - gLabelInfoBase + sizeof(LabelInfo) * 32) < 0) {
			bs_error(MSG_(BS_M_OUT_OF_MEMORY_LABEL));
			return -1;
		}
	}
	i = gLabelInfoTop / sizeof(LabelInfo);
	gLabelInfoTop += sizeof(LabelInfo);
	lbp = (LabelInfo *)gLabelInfoBasePtr + i;
	memset(lbp, 0, sizeof(LabelInfo));
	lbp->code = symcode;
	lbp->scope = funcidx;
	lbp->caddr = lbp->daddr = kInvalidProgPos;
	return i;
}

int
bs_lookup_var(SymbolCode symcode)
{
	int i = gVarInfoTop / sizeof(VarInfo) - 1;
	int funcidx = gParserInfo.fp - (FuncInfo *)gFuncInfoBasePtr;
	VarInfo *vp = (VarInfo *)gVarInfoBasePtr + i;
	for (; i >= 0; i--, vp--) {
		if (EqualSymbolCode(vp->code, symcode)) {
			/*  vp->scope: usually, 0 (global) or funcidx (local) are valid  */
			/*  During local var defnition, only funcidx (local) are valid (= duplicate def),
			   and 0 (global) var are treated as new symbol  */
			if (vp->scope == funcidx || (gParserInfo.local_var == 0 && vp->scope == 0))
				return i;
		}
	}
	return -1;
}

int
bs_define_dim(SymbolCode symcode)
{
	DimInfo *dp;
	if (gDimInfoTop >= gDimInfoEnd - gDimInfoBase) {
		if (bs_realloc_block(MEM_DIM_INFO, sizeof(DimInfo) * 16) < 0) {
			bs_error(MSG_(BS_M_OUT_OF_MEMORY_DIM_DEF));
			return -1;
		}
	}
	dp = (DimInfo *)(gDimInfoBasePtr + gDimInfoTop);
	gDimInfoTop += sizeof(DimInfo);
	memset(dp, 0, sizeof(DimInfo));
	dp->code = symcode;
	dp->offset = gParserInfo.varreq;
	gParserInfo.varreq += gTypeSize[BS_TYPE_STRING];  /*  Off_t: same size as String  */
	return gDimInfoTop / sizeof(DimInfo) - 1; /* The index to gDimInfo */
}

int
bs_lookup_dim(SymbolCode symcode)
{
	int i = gDimInfoTop / sizeof(DimInfo) - 1;
	DimInfo *dp = (DimInfo *)gDimInfoBasePtr + i;
	for (; i >= 0; i--, dp--) {
		if (EqualSymbolCode(dp->code, symcode))
			return i;
	}
	return -1;
}

int
bs_define_funcname(SymbolCode symcode, int is_func)
{
	FuncInfo *fp;
	if (gFuncInfoTop >= gFuncInfoEnd - gFuncInfoBase) {
		if (bs_realloc_block(MEM_FUNC_INFO, sizeof(FuncInfo) * 16) < 0) {
			bs_error(MSG_(BS_M_OUT_OF_MEMORY_FUNC_DEF));
			return -1;
		}
	}
	if (!is_func) {
		if (symcode.type != BS_TYPE_INTEGER) {
			bs_error(MSG_(BS_M_PROC_NAME_POSTFIX));
			return -1;
		}
		symcode.type = BS_TYPE_NONE;
	}
	
	fp = (FuncInfo *)(gFuncInfoBasePtr + gFuncInfoTop);
	gFuncInfoTop += sizeof(FuncInfo);
	memset(fp, 0, sizeof(FuncInfo));
	fp->code = symcode;
	fp->sign = 0;
	fp->saddr = fp->eaddr = kInvalidProgPos;
	return gFuncInfoTop / sizeof(FuncInfo) - 1;  /*  The index to gFuncInfo  */
}

/*  Look up PROC/FUNC name  */
/*  Returns index to the gFuncInfo array; 0 is an invalid entry; if the symbol
    is a PROC name, then 0x80000000 is bit-or'ed  */
int
bs_lookup_func(SymbolCode symcode)
{
	int i = gFuncInfoTop / sizeof(FuncInfo) - 1;
	FuncInfo *fp = (FuncInfo *)gFuncInfoBasePtr + i;
	for (; i > 0; i--, fp--) {
		/*  FuncInfo[0] is a dummy entry  */
		/*  EqualSymbolCode() is not used, because the 'type' field is used to
		    distinguish SUB and FUNC  */
		if (fp->code.str == symcode.str)
			return (fp->code.type == 0 ? (0x80000000 | i) : i);
	}
	return -1;
}

#if 0
#pragma mark ====== Tokenizer ======
#endif

int
bs_get_token(void)
{
	int ch, n, i, str, bufindex;
	
	/*  Get a single line  */
	do {
		if (s_bufindex < 0) {
			if (s_fpin != NULL) {
				/*  Direct compile from file  */
				if (fgets((char *)s_buf, s_bufsize, s_fpin) == NULL)
					return BS_EOF;  /*  End of file  */
			} else {
				/*  On memory  */
				for (i = 0; i < s_bufsize - 1; i++) {
					s_buf[i] = ch = s_pos[i];
					if (ch == '\n') {
						i++;
						break;
					} else if (ch == 0)
						break;
				}
				s_buf[i] = 0;
				s_pos += i;
				if (ch == 0)
					return BS_EOF;
			}
			gParserInfo.lineno++;
			s_bufindex = 0;
			n = strlen((char *)s_buf);
			if (s_buf[n - 1] != '\n') {
				s_bufindex = -1;
				bs_error(MSG_(BS_M_LINE_TOO_LONG), gParserInfo.lineno, s_bufsize - 2);
				return BS_ERROR_TOKEN;
			}
			
			/*  Record current execution address for each line  */
			if (gVMAddressTop >= gVMAddressEnd - gVMAddressBase - sizeof(Off_t)) {
				if (bs_realloc_block(MEM_VMADDRESS, gVMAddressEnd - gVMAddressBase + 1024) < 0) {
					bs_error(MSG_(BS_M_OUT_OF_MEMORY_LINE_ADDRESS), gParserInfo.lineno);
					return BS_EOF;
				}
			}
			*((Off_t *)(gVMAddressBasePtr + gVMAddressTop)) = gVMCodeTop;
			gVMAddressTop += sizeof(Off_t);
				
		}
		while ((ch = s_buf[s_bufindex++]) > 0 && ch <= ' ' && ch != '\n');
		if (ch == 0)
			s_bufindex = -1;
	} while (s_bufindex < 0);
	
	bufindex = s_bufindex - 1;  /*  The position of ch  */
	
	/*  Comment  */
	if (ch == '\'' || strncasecmp((char *)s_buf + bufindex, "REM", 3) == 0) {
		s_bufindex = -1;  /*  Discard the rest of the line  */
		return '\n';
	}

	/*  Symbol  */
	if (ch == '@' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
		SymbolCode sc;
		u_int8_t buf[BS_MAXSYMLEN + 4];
	
		/*  Reserved words  */
		n = bs_lookup_reserved_words((char *)s_buf + bufindex, &i);
		if (n >= 0) {
			s_bufindex = bufindex + i;
			return n;
		}
		
		/*  General Symbol  */
		n = 0;
		do {
			buf[n++] = (ch >= 'a' ? ch - ('a' - 'A') : ch);
		} while (n < sizeof buf - 3 &&
				 (((ch = s_buf[s_bufindex++]) >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')
				 || (ch >= '0' && ch <= '9')));
		if (n >= sizeof buf - 3) {
			bs_error(MSG_(BS_M_TOO_LONG_IDENTIFIER), sizeof buf - 4);
			s_bufindex = -1;
			return BS_ERROR_TOKEN;
		}
		buf[n] = 0;
		sc.type = BS_TYPE_INTEGER;
		switch (ch) {
			case BS_SUFFIX_FLT_CHAR: sc.type = BS_TYPE_FLOAT; break;
			case BS_SUFFIX_STR_CHAR: sc.type = BS_TYPE_STRING; break;
			default: s_bufindex--; ch = 0; break;
		}
		if (ch) {
			buf[n++] = ch;
			buf[n] = 0;
		}
		
		/*  Look for built-in function/procedure names  */
		n = bs_lookup_builtin_commands((char *)buf);
		if (n >= 0) {
			gTokenValue.u.iv = n;
			return BS_BUILTIN;
		}
		
		str = bs_new_literal_string((char *)buf, -1);
		if (str == kInvalidOff) {
			bs_error(MSG_(BS_M_OUT_OF_MEMORY_LITERAL_STRING));
			s_bufindex = -1;
			return BS_ERROR_TOKEN;
		}
		sc.str = str & ~kMSBOff;
		
		if (buf[0] == '@') {
			n = bs_lookup_label(sc);
			if (n >= 0) {
				gTokenValue.u.iv = n;
				return BS_LABEL;
			}
			/*  Error message is already shown  */
			s_bufindex = -1;
			return BS_ERROR_TOKEN;
		}
		
		n = bs_lookup_var(sc);
		if (n >= 0) {
			gTokenValue.u.iv = n;
			return BS_VARNAME;
		}

		if (gParserInfo.local_var == 0) {
			/*  Dimension and function names are skipped during local var definition  */
			n = bs_lookup_dim(sc);
			if (n >= 0) {
				gTokenValue.u.iv = n;
				return BS_DIMNAME;
			}
			n = bs_lookup_func(sc);
			if (n != -1) {
				gTokenValue.u.iv = (n & 0x7fffffff);
				return ((n & 0x80000000) ? BS_PROCNAME : BS_FUNCNAME);
			}
		}
		
		gTokenValue.u.symcode = sc;
		return BS_SYMBOL;
	}
	
	if (ch >= '0' && ch <= '9') {
		char *p = (char *)(s_buf + s_bufindex - 1);
		char *pp;
		for (pp = p + 1; *pp >= '0' && *pp <= '9'; pp++);
		if (*pp == '.' || *pp == 'E' || *pp == 'e') {
			/*  Floating point value  */
			gTokenValue.type = BS_TYPE_FLOAT;
			gTokenValue.u.fv = strtod(p, &p);
			s_bufindex = (u_int8_t *)p - s_buf;
			return BS_FLOAT;
		} else {
			/*  Integer value  */
			gTokenValue.type = BS_TYPE_INTEGER;
			gTokenValue.u.iv = strtoul(p, &p, 0);
			if (gTokenValue.u.iv == ULONG_MAX && errno == ERANGE) {
				bs_error(MSG_(BS_M_INTEGER_LITERAL_OVERFLOW));
				s_bufindex = -1;
				return BS_ERROR_TOKEN;
			}
			s_bufindex = (u_int8_t *)p - s_buf;
			return BS_INTEGER;
		}
	}
	if (ch == '\"' || ch == '\\') {
		/*  A string literal is a sequence of one or more string fragments.
		 A string fragment is either a string surrounded by two '\"'s,
		 or an escape sequence (\n, \r, \b, \t, \xHHHH..HH). */
		/*  The appearance of two consequent '\"'s is interpreted as as a single '\"'. */
		Off_t strval;
		strval = gConstStringTop + gConstStringBase;  /*  Use this memory area for temporary storage  */
		while (ch == '\"' || ch == '\\') {
			if (ch == '\"') {
				/*  Quoted string  */
				for (i = s_bufindex; s_buf[i] != 0; i++) {
					if (s_buf[i] == '\"') {
						if (s_buf[i + 1] != '\"')
							break;
						i++;
					}
					if (strval >= gConstStringEnd - 1) {
						bs_error(MSG_(BS_M_OUT_OF_MEMORY_LITERAL_STRING));
						s_bufindex = -1;
						return BS_ERROR_TOKEN;
					}
					gMemoryPtr[strval++] = s_buf[i];
				}
				if (s_buf[i] == 0) {
					bs_error(MSG_(BS_M_STRING_QUOTE_ERROR));
					s_bufindex = -1;
					return BS_ERROR_TOKEN;  /*  Skip error in the parser  */
				}
				i++;
			} else {
				/*  Escape sequence  */
				i = s_bufindex;
				ch = s_buf[i++];
				if (ch == 'x' || ch == 'X') {
					n = 0;
					while (1) {
						ch = s_buf[i];
						if (ch >= '0' && ch <= '9')
							ch -= '0';
						else if (ch >= 'A' && ch <= 'F')
							ch -= 'A' - 10;
						else if (ch >= 'a' && ch <= 'f')
							ch -= 'a' - 10;
						else break;
						i++;
						if (n == 0) {
							gMemoryPtr[strval] = (ch << 4);
							n = 1;
						} else {
							if (strval >= gConstStringEnd - 1) {
								if (bs_realloc_block(MEM_CONST_STRING, 1024) < 0) {
									bs_error(MSG_(BS_M_OUT_OF_MEMORY_LITERAL_STRING));
									s_bufindex = -1;
									return BS_ERROR_TOKEN;
								}
							}
							gMemoryPtr[strval++] += ch;
							n = 0;
						}
					}
					if (n == 1) {
						bs_error(MSG_(BS_M_WRONG_HEX_ESCAPE));
						s_bufindex = -1;
						return BS_ERROR_TOKEN;
					}
				} else {
					if (ch == 'n')
						ch = '\n';
					else if (ch == 'r')
						ch = '\r';
					else if (ch == 'b')
						ch = '\n';
					else if (ch == 't')
						ch = '\t';
					else {
						bs_error(MSG_(BS_M_WRONG_ESCAPE_SEQ));
						s_bufindex = -1;
						return BS_ERROR_TOKEN;
					}
					gMemoryPtr[strval++] = ch;
				}
			}
			while (s_buf[i] == ' ')
				i++;
			ch = s_buf[i];
			s_bufindex = i + 1;
		}
		gMemoryPtr[strval] = 0;
		strval = bs_new_literal_string((char *)gConstStringBasePtr + gConstStringTop, -1);
		gTokenValue.type = BS_TYPE_STRING;
		gTokenValue.u.sv = strval;
		s_bufindex--;
		return BS_STRING;
	}
	if (ch == '<') {
		if ((n = s_buf[s_bufindex]) == '=') {
			s_bufindex++;
			return BS_LTEQ;
		} else if (n == '>') {
			s_bufindex++;
			return BS_NEQ;
		} else return ch;
	}
	if (ch == '>') {
		if (s_buf[s_bufindex] == '=') {
			s_bufindex++;
			return BS_GTEQ;
		} else return ch;
	}
	if (ch == '!') {
		if (s_buf[s_bufindex] == '=') {
			s_bufindex++;
			return BS_NEQ;
		} else return ch;
	}
	if (ch > ' ' && ch < 0x80)
		return ch;
	if (ch == '\n' || ch == '\r')
		return '\n';
	bs_error(MSG_(BS_M_UNRECOGNIZED_CHAR), ch, (int)ch);
	s_bufindex = -1;
	return BS_ERROR_TOKEN;
}

int
yylex(void)
{
	
	int c = bs_get_token();
	yylval = gTokenValue;
	/*	printf("0x%02x %d %c\n", c, c, c); */
	return c;
}

void
yyerror(const char *p)
{
	bs_error("%s", p);
}

#if 0
#pragma mark ====== For Debug ======
#endif

#if 0

int
main(int argc, const char **argv)
{
	char buf[10240];
	int line = 0;
	int len, parse_result;
	extern int yydebug;
	bs_init_memory();
	parse_result = -1;
	while (fgets(buf, sizeof buf, stdin) != NULL) {
		if (buf[0] == '\n') {
			if (line != 0) {
				/*  Empty line: start compile  */
				bs_init_parser_memory();
				bs_start_parser_on_memory(gSourceBasePtr);
				parse_result = yyparse();
				bs_dump_vmcode(0, 0);
				bs_dump_vmdata(0, 0);
				line = 0;
			}
		} else if (strcmp(buf, "run\n") == 0) {
			if (parse_result == 0) {
				bs_execinit(0, 1);
				bs_execcode();
			} else {
				debug_printf("No VM code is present\n");
			}
		} else if (strcmp(buf, "tron\n") == 0) {
			bs_exec_set_trace_flag(1);
			debug_printf("Trace flag is now on\n");
		} else if (strcmp(buf, "troff\n") == 0) {
			bs_exec_set_trace_flag(0);
			debug_printf("Trace flag is now off\n");
		} else if (strcmp(buf, "yydebugon\n") == 0) {
			yydebug = 1;
			debug_printf("yydebug is now on\n");
		} else if (strcmp(buf, "yydebugoff\n") == 0) {
			yydebug = 0;
			debug_printf("yydebug is now off\n");
		} else {
			if (line == 0) {
				gSourceTop = 0;
				parse_result = -1;
			}
			len = strlen(buf);
			if (gSourceTop + len + 1 >= gSourceEnd - gSourceBase) {
				if (bs_realloc_block(MEM_SOURCE, gSourceEnd - gSourceBase + 4096) < 0) {
					fprintf(stderr, "Out of memory\n");
					exit(1);
				}
			}
			strcpy((char *)gSourceBasePtr + gSourceTop, buf);
			gSourceTop += len;
			line++;
		}
	}
}

#endif
