/*
 *  memory.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 16/02/15.
 *  Copyright 2016 Toshi Nagata. All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "daruma.h"

#define SOURCE_SIZE       (12*1024)
#define VMTABLE_SIZE      (1*1024)
#define VMCODE_SIZE       (8*1024)
#define VMDATA_SIZE       (8*1024)
#define VMADDRESS_SIZE    (4*1024)
#define CONST_STRING_SIZE (4*1024)
#define SYMBOL_NAME_SIZE  (4*1024)
#define VAR_INFO_SIZE     (sizeof(VarInfo)*256)
#define DIM_INFO_SIZE     (sizeof(DimInfo)*128)
#define LABEL_INFO_SIZE   (sizeof(LabelInfo)*128)
#define FUNC_INFO_SIZE    (sizeof(FuncInfo)*128)
#define LOOP_INFO_SIZE    (sizeof(LoopInfo)*256)
#define TEMP_SIG_SIZE     (4*1024)
#define VAR_SIZE          (12*1024)
#define STRING_CELLS_SIZE (1*1024)
#define STRING_SIZE       (4*1024)
#define PATINFO_SIZE      (2*1024)
#define PATTERN_SIZE      (8*1024)
#define VMSTACK_SIZE      (4*1024)
#define PARSE_STACK_SIZE  (4*1024)

const Off_t sInitialSizes[] = {
	SOURCE_SIZE, VMTABLE_SIZE, VMCODE_SIZE, VMDATA_SIZE, VMADDRESS_SIZE,
	CONST_STRING_SIZE, SYMBOL_NAME_SIZE, VAR_INFO_SIZE, DIM_INFO_SIZE,
	LABEL_INFO_SIZE, FUNC_INFO_SIZE, LOOP_INFO_SIZE, TEMP_SIG_SIZE,
	VAR_SIZE, STRING_CELLS_SIZE, STRING_SIZE, 
	PATINFO_SIZE, PATTERN_SIZE, VMSTACK_SIZE, PARSE_STACK_SIZE, 0 };

/*  NOTE: PARSE_STACK_SIZE should be no larger than YYSTACK_BYTES (YYMAXDEPTH). */
/*  See y.tab.c for details.  */

/*  Definition of global variables  */

u_int8_t *gMemoryPtrs[MEM_END + 1];
Off_t gMemoryOffsets[MEM_END + 1];
size_t gMemorySize;

Off_t gSourceTop, gVMCodeTop, gVMDataTop, gVMAddressTop, gConstStringTop;
Off_t gVarInfoTop, gDimInfoTop, gLabelInfoTop, gFuncInfoTop, gLoopInfoTop, gTempSigTop;
Off_t gVarTop, gPatInfoTop, gPatternTop;

ParserInfo gParserInfo;

u_int8_t gSourceTouched;
u_int8_t gRunMode;

/*  Size of simple types  */
const u_int8_t gTypeSize[8] = {
	0,
	sizeof(Int),
	sizeof(Float),
	sizeof(Off_t),
	0, 0, 0, 0 };

/*  bs_init_parser_memory()
   Initialize the working memory for the parser. All existing information regarding the
   parser is cleared.  */
int
bs_init_parser_memory(void)
{
	int i;
	gVMTableEnd = gVMTableBase + VMTABLE_SIZE;
	gVMCodeEnd = gVMCodeBase + VMCODE_SIZE;
	gVMDataEnd = gVMDataBase + VMDATA_SIZE;
	gVMAddressEnd = gVMAddressBase + VMADDRESS_SIZE;
	gConstStringEnd = gConstStringBase + CONST_STRING_SIZE;
	gVarInfoEnd = gVarInfoBase + VAR_INFO_SIZE;
	gDimInfoEnd = gDimInfoBase + DIM_INFO_SIZE;
	gLabelInfoEnd = gLabelInfoBase + LABEL_INFO_SIZE;
	gFuncInfoEnd = gFuncInfoBase + FUNC_INFO_SIZE;
	gLoopInfoEnd = gLoopInfoBase + LOOP_INFO_SIZE;
	gTempSigEnd = gTempSigBase + TEMP_SIG_SIZE;
	gVarEnd = gVarBase;
	gStringCellsEnd = gStringCellsBase;
	gStringEnd = gStringBase;
	gPatInfoEnd = gPatInfoBase;
	gPatternEnd = gPatternBase;
	gVMStackEnd = gVMStackBase;
	gParseStackEnd = gParseStackBase + PARSE_STACK_SIZE;

	for (i = MEM_VMTABLE; i <= MEM_END; i++)
		gMemoryPtrs[i] = gMemoryPtrs[0] + gMemoryOffsets[i];
	memset(gMemoryPtrs[MEM_VMTABLE], 0, gMemoryOffsets[MEM_END] - gMemoryOffsets[MEM_VMTABLE]);

#if 0 && __circle__
	log_printf("%s:%d:gMemoryPtrs[0] = %08x\n", __FILE__, __LINE__, (uint32_t)gMemoryPtrs[0]);
#endif
	
	
	gVMCodeTop = 0;
	gVMDataTop = 0;
	gVMAddressTop = 0;
	gVarInfoTop = gDimInfoTop = gLabelInfoTop = gFuncInfoTop = gLoopInfoTop = 0;

	/*  tempsig starts from 1 (0 is reserved for DATA statements)  */
	gTempSigTop = sizeof(TempSignature);

	/*  ConstString[0] is always a NULL string  */
	gConstStringBasePtr[0] = 0;
	gConstStringTop = 1;

	memset(&gParserInfo, 0, sizeof(gParserInfo));

	/*  gFuncInfo[0] is a dummy entry  */
	gParserInfo.fp = (FuncInfo *)gFuncInfoBasePtr;
	gFuncInfoTop = sizeof(FuncInfo);

	gParserInfo.ifaddr = kInvalidProgPos;
	gParserInfo.dataaddr = kInvalidProgPos;
	
	/*  No temporary signature  */
	gParserInfo.tempsig = 0;
	
	return 0;
}

/*  bs_init_memory()
    Allocate main memory and initialize the global variables  */
int
bs_init_memory(void)
{
	gMemorySize = 1024 * 1024;

	gMemoryPtr = (u_int8_t *)bs_malloc(gMemorySize);
	gSourceBase = 0;
	gVMTableBase = gSourceBase + SOURCE_SIZE;
	gSourceBasePtr = gMemoryPtr + gSourceBase;
	gVMTableBasePtr = gMemoryPtr + gVMTableBase;
	
	bs_init_parser_memory();

	return 0;
}

/*  bs_init_runtime_memory()
    Initialize the runtime memory  */
int
bs_init_runtime_memory(void)
{
	if (bs_realloc_block(MEM_TEMP_SIG, 0) ||
		bs_realloc_block(MEM_VAR, VAR_SIZE) ||
		bs_realloc_block(MEM_STRING_CELLS, STRING_CELLS_SIZE) ||
		bs_realloc_block(MEM_STRING, STRING_SIZE) ||
		bs_realloc_block(MEM_VMSTACK, VMSTACK_SIZE))
		return -1; /*  Cannot allocate  */
	bs_init_runtime_string();
	return 0;
}

/*  bs_realloc_block(index, size)
    Expand or shrink the index-th memory block  */
int
bs_realloc_block(int index, int size)
{
	int32_t dsize, i;
	if (index < 0 || index >= MEM_END)
		return 0;  /*  Do nothing  */
	size = (size + (CHUNK_SIZE - 1)) / CHUNK_SIZE * CHUNK_SIZE;
	dsize = size - (gMemoryOffsets[index + 1] - gMemoryOffsets[index]);
	if (gMemoryOffsets[MEM_END] + dsize > gMemorySize) {
		/*  Expand the memory block  */
		size_t new_size = gMemorySize + 1024 * 1024;
		u_int8_t *p = (u_int8_t *)bs_realloc(gMemoryPtr, new_size);
		if (p == NULL)
			return -1;  /*  Out of memory  */
		gMemorySize = new_size;
		gMemoryPtr = p;
	}
	if (index < MEM_END - 1)
		memmove(gMemoryPtr + gMemoryOffsets[index + 1] + dsize, gMemoryPtr + gMemoryOffsets[index + 1], gMemoryOffsets[MEM_END] - gMemoryOffsets[index + 1]);
	if (dsize > 0)
		memset(gMemoryPtrs[index + 1], 0, dsize);
	for (i = index + 1; i <= MEM_END; i++) {
		gMemoryOffsets[i] += dsize;
		gMemoryPtrs[i] += dsize;
	}
	
	if (index < MEM_VMSTACK) {
		/*  Relocate runtime stack pointers (which are cached within execcode.c)  */
		bs_relocate_runtime_stack(dsize);
	}
	
	return 0;
}
