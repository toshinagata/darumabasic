/*
 *  daruma.h
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 2015/12/17.
 *  Copyright 2015 Toshi Nagata.  All rights reserved.
 *
 */

#ifndef __daruma_h__
#define __daruma_h__

#include <stdint.h>
#include <sys/types.h>
#include <sys/param.h>  /*  For MAXPATHLEN  */

/*  To enable debug_printf, define ENABLE_DPRINTF  */
#if defined(ENABLE_DEBUG_PRINTF)
extern int debug_printf(const char *fmt, ...);
#else
#define debug_printf(...)
#endif

/*  Standard types  */
typedef int32_t Int;  /*  Integer  */
typedef double Float; /*  Floating point number  */
typedef Int Off_t;    /*  Offset type: same as integer  */

#define SIZEOF_FLOAT 8

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef u_int32_t u_int32_t;

extern volatile u_int8_t gTimerInvoked;
extern volatile int32_t gTimerCount;

#define BS_OFF_T_IS_32BIT 1

#if BS_OFF_T_IS_32BIT
#define kInvalidOff 0xffffffff
#define kMSBOff     0x80000000
#define kInvalidProgPos ((Off_t)0xffffff)
#define SIZEOF_PROGPOS 3
#else
#define kInvalidOff 0xffff
#define kMSBOff     0x8000
#define kInvalidProgPos ((Off_t)0xffff)
#define SIZEOF_PROGPOS 2
#endif

#if 0
#pragma mark ====== Memory Implementation ======
#endif

/*  Memory map  */
/*
-------- gStackEnd
CPU Stack
-------- gStackBase
-------- gParseStackEnd
Semantic Stack
-------- gParseStackBase
-------- gVMStackEnd
VM Runtime Stack
-------- gVMStackBase
-------- gPatternEnd
Pattern Data
-------- gPatternBase
-------- gPatInfoEnd
Pattern Info (LPattern)
-------- gPatInfoBase
-------- gStringEnd
Runtime String
-------- gStringBase
Runtime String Cells
-------- gStringCellsBase
-------- gVarEnd
Runtime Variable
-------- gVarBase
*** Above here is used at run-time
*** Below here is determined at compile-time
-------- gTempSigEnd
Temp Sig
-------- gTempSigBase
-------- gLoopInfoEnd
Loop Infos
-------- gLoopInfoBase
-------- gFuncInfoEnd
Func Infos
-------- gFuncInfoBase
-------- gLabelInfoEnd
Label Infos
-------- gLabelInfoBase
-------- gDimInfoEnd
Dim Infos
-------- gDimInfoBase
-------- gVarInfoEnd
Var Infos
-------- gVarInfoBase
-------- gConstStringEnd
String Literals
-------- gConstStringBase
-------- gVMAddressEnd
VM address
-------- gVMAddressBase
-------- gVMDataEnd
VM Data
-------- gVMDataBase
-------- gVMCodeEnd
........ gVMCodeDirectBase <- Base position of VM code for direct execution
VM Code
-------- gVMCodeBase
-------- gVMTableEnd
List of Runtime Values of gXXXXXX
-------- gVMTableBase
-------- gSourceEnd (SOURCE_SIZE: 12K)
Source
-------- gSourceBase (= 0)
*/

enum {
	MEM_SOURCE = 0,
	MEM_VMTABLE,
	MEM_VMCODE,
	MEM_VMDATA,
	MEM_VMADDRESS,
	MEM_CONST_STRING,
	MEM_VAR_INFO,
	MEM_DIM_INFO,
	MEM_LABEL_INFO,
	MEM_FUNC_INFO,
	MEM_LOOP_INFO,
	MEM_TEMP_SIG,
	MEM_VAR,
	MEM_STRING_CELLS,
	MEM_STRING,
	MEM_PATINFO,
	MEM_PATTERN,
	MEM_VMSTACK,
	MEM_PARSE_STACK,
	MEM_END
};

extern u_int8_t *gMemoryPtrs[MEM_END + 1];
/*extern u_int8_t *gMemoryPtr;  *//*  Base address of the free RAM  */
extern Off_t gMemoryOffsets[MEM_END + 1];
extern size_t gMemorySize;   /*  Size of allocated memory  */

/*  Size for aligning memory block  */
#define CHUNK_SIZE 8

#define gMemoryPtr          (gMemoryPtrs[0])
#define gSourceBasePtr      (gMemoryPtrs[MEM_SOURCE])
#define gVMTableBasePtr     (gMemoryPtrs[MEM_VMTABLE])
#define gVMCodeBasePtr      (gMemoryPtrs[MEM_VMCODE])
#define gVMDataBasePtr      (gMemoryPtrs[MEM_VMDATA])
#define gVMAddressBasePtr   (gMemoryPtrs[MEM_VMADDRESS])
#define gConstStringBasePtr (gMemoryPtrs[MEM_CONST_STRING])
#define gVarInfoBasePtr     (gMemoryPtrs[MEM_VAR_INFO])
#define gDimInfoBasePtr     (gMemoryPtrs[MEM_DIM_INFO])
#define gLabelInfoBasePtr   (gMemoryPtrs[MEM_LABEL_INFO])
#define gFuncInfoBasePtr    (gMemoryPtrs[MEM_FUNC_INFO])
#define gLoopInfoBasePtr    (gMemoryPtrs[MEM_LOOP_INFO])
#define gTempSigBasePtr     (gMemoryPtrs[MEM_TEMP_SIG])
#define gVarBasePtr         (gMemoryPtrs[MEM_VAR])
#define gStringCellsBasePtr (gMemoryPtrs[MEM_STRING_CELLS])
#define gStringBasePtr      (gMemoryPtrs[MEM_STRING])
#define gPatInfoBasePtr     (gMemoryPtrs[MEM_PATINFO])
#define gPatternBasePtr     (gMemoryPtrs[MEM_PATTERN])
#define gVMStackBasePtr     (gMemoryPtrs[MEM_VMSTACK])
#define gParseStackBasePtr  (gMemoryPtrs[MEM_PARSE_STACK])

#define gVMStackEndPtr      (gMemoryPtrs[MEM_VMSTACK+1])

#define gSourceBase      (gMemoryOffsets[MEM_SOURCE])
#define gVMTableBase     (gMemoryOffsets[MEM_VMTABLE])
#define gVMCodeBase      (gMemoryOffsets[MEM_VMCODE])
#define gVMDataBase      (gMemoryOffsets[MEM_VMDATA])
#define gVMAddressBase   (gMemoryOffsets[MEM_VMADDRESS])
#define gConstStringBase (gMemoryOffsets[MEM_CONST_STRING])
#define gVarInfoBase     (gMemoryOffsets[MEM_VAR_INFO])
#define gDimInfoBase     (gMemoryOffsets[MEM_DIM_INFO])
#define gLabelInfoBase   (gMemoryOffsets[MEM_LABEL_INFO])
#define gFuncInfoBase    (gMemoryOffsets[MEM_FUNC_INFO])
#define gLoopInfoBase    (gMemoryOffsets[MEM_LOOP_INFO])
#define gTempSigBase     (gMemoryOffsets[MEM_TEMP_SIG])
#define gVarBase         (gMemoryOffsets[MEM_VAR])
#define gStringCellsBase (gMemoryOffsets[MEM_STRING_CELLS])
#define gStringBase      (gMemoryOffsets[MEM_STRING])
#define gPatInfoBase     (gMemoryOffsets[MEM_PATINFO])
#define gPatternBase     (gMemoryOffsets[MEM_PATTERN])
#define gVMStackBase     (gMemoryOffsets[MEM_VMSTACK])
#define gParseStackBase  (gMemoryOffsets[MEM_PARSE_STACK])

#define gSourceEnd      (gMemoryOffsets[MEM_SOURCE+1])
#define gVMTableEnd     (gMemoryOffsets[MEM_VMTABLE+1])
#define gVMCodeEnd      (gMemoryOffsets[MEM_VMCODE+1])
#define gVMDataEnd      (gMemoryOffsets[MEM_VMDATA+1])
#define gVMAddressEnd   (gMemoryOffsets[MEM_VMADDRESS+1])
#define gConstStringEnd (gMemoryOffsets[MEM_CONST_STRING+1])
#define gVarInfoEnd     (gMemoryOffsets[MEM_VAR_INFO+1])
#define gDimInfoEnd     (gMemoryOffsets[MEM_DIM_INFO+1])
#define gLabelInfoEnd   (gMemoryOffsets[MEM_LABEL_INFO+1])
#define gFuncInfoEnd    (gMemoryOffsets[MEM_FUNC_INFO+1])
#define gLoopInfoEnd    (gMemoryOffsets[MEM_LOOP_INFO+1])
#define gTempSigEnd     (gMemoryOffsets[MEM_TEMP_SIG+1])
#define gVarEnd         (gMemoryOffsets[MEM_VAR+1])
#define gStringCellsEnd (gMemoryOffsets[MEM_STRING_CELLS+1])
#define gStringEnd      (gMemoryOffsets[MEM_STRING+1])
#define gPatInfoEnd     (gMemoryOffsets[MEM_PATINFO+1])
#define gPatternEnd     (gMemoryOffsets[MEM_PATTERN+1])
#define gVMStackEnd     (gMemoryOffsets[MEM_VMSTACK+1])
#define gParseStackEnd  (gMemoryOffsets[MEM_PARSE_STACK+1])

/*  The following members are expressed as offsets from the 'base' pointer of each block  */
/*  gVMCodeTop is for code generation, whereas sVMCodePos (see below) is for execution  */
extern Off_t gSourceTop, gVMCodeTop, gVMDataTop, gVMAddressTop, gConstStringTop;
extern Off_t gVarInfoTop, gDimInfoTop, gLabelInfoTop, gFuncInfoTop, gLoopInfoTop, gTempSigTop;
extern Off_t gVarTop, gPatInfoTop, gPatternTop;

/*  Init memory  */
extern int bs_init_memory(void);
extern int bs_init_parser_memory(void);
extern int bs_init_runtime_memory(void);
extern int bs_realloc_block(int index, int size);
extern void bs_relocate_runtime_stack(Off_t dsize);
extern int bs_exec_set_trace_flag(int flag);

/*  True if the on-memory program is modified  */
extern u_int8_t gSourceTouched;

enum {
	BS_RUNMODE_NONE,
	BS_RUNMODE_PARSE,
	BS_RUNMODE_DIRECT,
	BS_RUNMODE_BATCH
};

extern u_int8_t gRunMode;

/*  List/Edit/Load/Save  */
extern int bs_list(int sline, int eline);
extern int bs_load_file(const char *path);
extern int bs_save_file(const char *path);
extern int bs_edit(void);

extern int bs_builtin_list(void);
extern int bs_builtin_save(void);
extern int bs_builtin_load(void);

/*  Base directory  */
extern char bs_basedir[MAXPATHLEN];

/*  Current file name (relative to the base directory, or absolute path)  */
extern char bs_filename[MAXPATHLEN];

/*  Reserved words  */
/*  gReservedWordsString is a constant string containing NUL-delimited
    reserved words.
    gReservedWords[N] contains the internal code of the N-th reserved word.  */
extern const char gReservedWordsString[];
extern const Int gReservedWords[];

#if 0
#pragma mark ====== String Implementation ======
#endif

/*  String Values  */
/*  A string value is stored either in the string literal section
    or in the runtime string section.
 
    A runtime string is represented as an offset from (gMemoryPtr + gStringCellsBase),
    and a literal string is represented as an offset from (gMemoryPtr + gConstStringBase)
    plus 0x8000 (if Off_t is 16 bit) or 0x80000000.

    A literal string is represented as a direct pointer, whereas a 
    runtime string is a pointer to one of the cells located in the 
    lower part of the runtime string section. The cells retains a
    pointer to the real string with a reference count.

	The free space in the runtime string section is managed by a linked list.
    The first 1~4 bytes denote the size of this free space as a compressed
    integer, and the following 2 (or 4) bytes retains the pointer to the next 
    free space. The first byte of the free space is always non-zero, because
    the space size should never be zero, and non-zero compressed integer should
    never begin with zero.
 
    When a string becomes free, the space is registered as a free space,
    and the cell containing the pointer is marked 'unused' by kInvalidOff.
 
    When a new string is requested, then the free space is searched for
    the requested amount of space. If no space is present, then garbage
    collection is invoked.
*/

extern Off_t gStringFreeRoot;

extern void bs_init_runtime_string(void);
extern Off_t bs_allocate_literal_string(const char *cs, int len);
extern Off_t bs_new_literal_string(const char *cs, int len);
extern Off_t bs_new_runtime_string(const char *cs, int len);
extern Off_t bs_allocate_runtime_string(int len);
extern Off_t bs_retain_string(Off_t str);
extern Off_t bs_release_string(Off_t str);
extern char *bs_get_string_ptr(Off_t str);

extern Off_t bs_put_compressed_integer(Off_t pos, u_int32_t num);
extern Off_t bs_get_compressed_integer(Off_t pos, u_int32_t *nump);

#if 0
#pragma mark ====== Symbol Implementation ======
#endif

/*  The following macros assumes y.tab.h is already included, and
    BS_IF is the first token in y.tab.h  */
#define CODE_BASE 0x80
#define C(t) (u_int8_t)((t) - BS_IF + CODE_BASE)
#define T(c) ((int)(c) - CODE_BASE + BS_IF)

/*  Maximum length of a symbol  */
#define BS_MAXSYMLEN 32

enum {
	BS_SUFFIX_FLT_CHAR = '#',
	BS_SUFFIX_STR_CHAR = '$',
	BS_SUFFIX_BYTE_CHAR = '%'
};

enum {
	BS_TYPE_NONE = 0,
	BS_TYPE_INTEGER = 1,
	BS_TYPE_FLOAT = 2,
	BS_TYPE_STRING = 3,
	BS_TYPE_NOVALUE = 7,
	BS_TYPE_LOCAL = 8    /*  Local variable: only used in yacc values for lvalues  */
};

typedef struct SymbolCode {
	int   type:8;
	Off_t str:24;  /*  Offset from gConstStringBase  */
} SymbolCode;

#define EqualSymbolCode(sc1, sc2) (*((int32_t *)&sc1) == *((int32_t *)&sc2))

extern const u_int8_t gTypeSize[8];
extern const char *gTypeNames[4];

extern int bs_lookup_reserved_words(char *name, int *len);
extern int bs_lookup_builtin_commands(char *name);

extern int bs_define_var(SymbolCode symcode, int scope);
extern int bs_define_dim(SymbolCode symcode);
extern int bs_define_funcname(SymbolCode symcode, int is_func);

#if 0
#pragma mark ====== Built-in Functions/Commands ======
#endif

typedef struct VarInfo {
	SymbolCode code;
	Off_t offset;
	int16_t scope;
} VarInfo;

typedef struct DimInfo {
	SymbolCode code;
	Off_t offset;
	int16_t dim;
} DimInfo;

typedef struct LabelInfo {
	SymbolCode code;
	u_int8_t defined;  /*  Is the label already defined?  */
	int16_t scope;     /*  The func/sub (or global) to which this label belongs  */
	Off_t caddr;       /*  Address of the code section  */
	Off_t daddr;       /*  Address of the data section  */
} LabelInfo;

typedef struct FuncInfo {
	SymbolCode code;
	u_int8_t defined;  /*  0: newly defined, 1: used but not defined, 2: defined  */
	Off_t sign;        /*  Argument signature (offset from gConstStringBasePtr)  */
	Off_t lsize;       /*  Size of local variable area (in bytes)  */
	Off_t ltop;        /*  Top of present local variable (in bytes)  */
	Off_t saddr;       /*  Start address  */
	Off_t eaddr;       /*  End address  */
} FuncInfo;

/*  Maximum size for local variables and function arguments  */
#define BS_LOCAL_VAR_LIMIT  8000

typedef struct BuiltInInfo {
	int type;   /*  Return type: BS_TYPE_NONE = commands  */
	const char *name;  /*  Function/Command name  */
	int (*entry)(void);    /*  Function entry point  */
	const u_int8_t *sign;  /*  Argument signature  */
	const u_int8_t *dimsign;  /*  DIM argument signature  */
} BuiltInInfo;

/*  External declaration for built-in functions  */
extern const BuiltInInfo gBuiltIns[];
extern const int gNumberOfBuiltIns;

extern void bs_reset_arg_accessor(void);
extern int bs_get_next_int_arg(Int *ip);
extern int bs_get_next_float_arg(Float *fp);
extern int bs_get_next_str_arg(Off_t *sp);
extern int bs_get_next_arg_type_and_ptr(void **outptr);

extern void bs_push_int_value(Int ival);
extern void bs_push_float_value(Float dval);
extern void bs_push_str_value(Off_t sval);

#define NEXT_INT_ARG *((const Int *)bs_get_next_arg_ptr())
#define NEXT_FLT_ARG *((const Float *)bs_get_next_arg_ptr())
#define NEXT_STR_ARG *((const Off_t *)bs_get_next_arg_ptr())

enum {
	BS_LOOPTYPE_FOR = 1,
	BS_LOOPTYPE_DO,
	BS_LOOPTYPE_DO_WHILE,
	BS_LOOPTYPE_DO_UNTIL,
	BS_LOOPTYPE_LOOP_WHILE,
	BS_LOOPTYPE_LOOP_UNTIL
};

enum {
	BS_LOOPFLAG_SMASK = 1,
	BS_LOOPFLAG_EMASK = 2,
	BS_LOOPFLAG_CMASK = 4,
	BS_LOOPFLAG_ALLMASK = (BS_LOOPFLAG_SMASK | BS_LOOPFLAG_EMASK | BS_LOOPFLAG_CMASK),
	BS_LOOPFLAG_ISFLOAT = 8,
	BS_LOOPFLAG_ISLOCAL = 16
};

typedef struct LoopInfo {
	u_int8_t type;     /*  FOR/DO/DO_WHILE/DO_UNTIL/LOOP_WHILE/LOOP_UNTIL  */
	u_int8_t flags;    /*  bit 0/1/2: 1:address is undefined, 0:defined; 0=start, 1=end, 2=cont
						   bit 3: type of FOR variable, 0:int, 1:float
						   bit 4: scope of FOR variable, 0:global, 1:local */
	Off_t    coff;     /*  Control variables (address of the variable, final value, step)  */
	Off_t    saddr;    /*  Start address  */
	Off_t    eaddr;    /*  End address  */
	Off_t    caddr;    /*  Continue address  */
} LoopInfo;

/*  Data array accessor  */
/*  'Data array' is an array of values with various types.
 Three different types can be stored in the array: integer, float, and string.
 The type of N'th value is stored in the signature string as small type codes
 (1: integer, 2: float, 3: string). The type code of the N'th value is found
 in bits [2Y,2Y+1] of X-th byte, where X = N/4 and Y = N%4, in the signature string. */
/*  To access to the N'th value at minimum time, an accessor keeps the index to the
 'last accessed' value; actually, idx is index*2, and ofs is the offset from the
 top of the data pointer.  To proceed one index, idx is incremented by 2, the 
 value type is obtained as (signature[idx/8]>>(idx%8))&3, and after getting the 
 actual value, ofs is incremented by the value size.  */
typedef struct DataAccessor {
	int16_t type; /* Used only for internal temporary cache */
	int16_t idx;
	int16_t ofs;
} DataAccessor;

#define TYPE_OF_DATA(sig, acc) (acc.type = (sig[acc.idx / 8] >> (acc.idx % 8)) & 3)
#define NEXT_DATA(sig, acc) (TYPE_OF_DATA(sig, acc) > 0 ? \
(acc.ofs += gTypeSize[acc.type], acc.idx += 2, acc.type) : 0)
#define LAST_DATA(sig, acc) (acc.idx >= 0 ? \
(acc.idx -= 2, acc.ofs -= gTypeSize[TYPE_OF_DATA(sig, acc)], acc.type) : 0)
#define ADD_DATA_SIG(sig, acc, type) \
(sig[acc.idx / 8] |= (type << (acc.idx % 8)), \
acc.ofs += gTypeSize[type], acc.idx += 2)
#define INIT_DATA(acc) (acc.ofs = acc.idx = 0)

typedef struct TempSignature {
	DataAccessor acc;        /*  Data accessor  */
	DataAccessor acc_callee; /*  Accessor for callee signature  */
	Off_t    funcidx;        /*  Index to callee func/sub index; if negative, then
							  -(funcidx+1) is the index to gBuiltIns  */
	Off_t    incpos;         /*  VMCode position for increasing stack pointer  */
	u_int8_t sigbuf[64];     /*  Temporary signature  */
} TempSignature;

/*  Parser status  */
typedef struct ParserInfo {
	Off_t lineno;    /*  Line number  */
	Off_t errcnt;    /*  Error count  */
	Off_t varreq;    /*  The requested size for global variable  */
	Off_t ifaddr;    /*  IF jump addresses (making links during compilation of IF statements) */
	Off_t tempsig;   /*  Temporary signature (gTempSigBase index; 0 if no temporary sig
					     (gTempSigBase[0] is used for DATA statement)  */
	Off_t dataaddr;  /*  DATA or CDATA address (for DATA/CDATA statements)  */
	Off_t dim_index; /*  Current DIM index (used in DIM initializer)  */
	u_int8_t local_var;  /*  Local vars are being defined; 1:formal arguments, 2: LOCAL statement  */
	FuncInfo *fp;    /*  Current FUNC/PROC info  */
	LoopInfo *lp;    /*  Current loop info  */
} ParserInfo;

extern ParserInfo gParserInfo;

#if 0
#pragma mark ====== Token Value Declaration ======
#endif

/*  Token value  */
typedef struct TokenValue {
	int16_t type;  /*  Type of value  */
	union {
		Int iv;
		Float fv;
		Off_t sv;
		SymbolCode symcode;
	} u;
} TokenValue;

extern TokenValue gTokenValue;
extern TokenValue gZeroTokenValue;

#define YYSTYPE TokenValue

extern int yylex(void);
extern void yyerror(const char *p);
extern int yyparse(void);

/*  Controlling error output  */
#define yytnamerr my_yytnamerr
extern unsigned int my_yytnamerr(char *yyres, const char *yystr);

extern int bs_start_parser_on_memory(const u_int8_t *pos, int new_run);

extern void bs_runtime_error(const char *fmt, ...);
extern void bs_error(const char *fmt, ...);
extern int bs_runloop(void);

extern u_int64_t bs_uptime(int init);

#endif /* __pibasic_h__ */
