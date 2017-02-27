/*
 *  string.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 16/02/15.
 *  Copyright 2016 Toshi Nagata. All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "daruma.h"

Off_t gStringFreeRoot;

/*  Get free space size and next pointer  */
static Off_t
s_bs_get_free_space_info(Off_t pos, u_int32_t *size)
{
	/*  Let pos be offset from gMemoryPtr, not gMemoryPtr + gStringBase  */
	pos += gStringBase;
	pos = bs_get_compressed_integer(pos, size);
#if BS_OFF_T_IS_32BIT
	return (u_int32_t)gMemoryPtr[pos] + 256 * ((u_int32_t)gMemoryPtr[pos + 1] +
											256 * ((u_int32_t)gMemoryPtr[pos + 2] +
												   256 * (u_int32_t)gMemoryPtr[pos + 3]));
#else
	return gMemoryPtr[pos] + 256 * (u_int16_t)gMemoryPtr[pos + 1];
#endif
}

static void
s_bs_put_free_space_info(Off_t pos, u_int32_t size, Off_t npos)
{
	/*  Let pos be offset from gMemoryPtr, not gMemoryPtr + gStringBase  */
	pos += gStringBase;

	/*  If size is zero, then skip the size part  */
	if (size > 0)
		pos = bs_put_compressed_integer(pos, size);
	else {
		while (gMemoryPtr[pos++] >= 0x80);
	}
	gMemoryPtr[pos++] = (npos & 0xff);
	gMemoryPtr[pos++] = ((npos >> 8) & 0xff);
#if BS_OFF_T_IS_32BIT
	gMemoryPtr[pos++] = ((npos >> 16) & 0xff);
	gMemoryPtr[pos++] = ((npos >> 24) & 0xff);
#endif
}

void
bs_init_runtime_string(void)
{
	int i;
	s_bs_put_free_space_info(0, gStringEnd - gStringBase, kInvalidOff);
	gStringFreeRoot = 0;
	for (i = gStringCellsBase; i < gStringCellsEnd; i += sizeof(Off_t)) {
		*((Off_t *)(gMemoryPtr + i)) = kInvalidOff;
	}
}

Off_t
bs_allocate_literal_string(const char *cs, int len)
{
	Off_t str;
	if (gConstStringTop + len + 1 >= gConstStringEnd - gConstStringBase) {
		if (bs_realloc_block(MEM_CONST_STRING, (gConstStringEnd - gConstStringBase) * 2) < 0)
			return kInvalidOff;
	}
	if (cs != NULL)
		strncpy((char *)gConstStringBasePtr + gConstStringTop, cs, len);
	else
		memset((char *)gConstStringBasePtr, 0xff, len);
	str = gConstStringTop;
	gConstStringTop += len;
	gConstStringBasePtr[gConstStringTop] = 0;
	gConstStringTop++;
	return str | kMSBOff;
}

Off_t
bs_new_literal_string(const char *cs, int len)
{
	Off_t str;

	if (cs == NULL || *cs == 0) {
		return 0 | kMSBOff;  /*  ConstString[0] is always a NULL string  */
	}
	if (len == -1)
		len = strlen(cs);
	/*  Look for the same string  */
	str = 1;
	while (str < gConstStringTop) {
		if (strcmp((char *)gConstStringBasePtr + str, cs) == 0) {
			/*  Return the existing literal  */
			return str | kMSBOff;
		}
		str += strlen((char *)gConstStringBasePtr + str) + 1;
	}
	/*  Not found: allocate a new literal string  */
	return bs_allocate_literal_string(cs, len);
}

Off_t
bs_allocate_runtime_string(int len)
{
	u_int32_t size;
	Off_t pos, pos2, pos3;
	int i, len2;
	u_int8_t *p;
	
	/*  Add +2 for end mark and reference counter  */
	len += 2;
	
	/*  Look for the free space  */
	pos = gStringFreeRoot;
	pos3 = kInvalidOff;
	while (pos != kInvalidOff) {
		pos2 = s_bs_get_free_space_info(pos, &size);
		if (size >= len)
			goto found;
		pos3 = pos;
		pos = pos2;
	}

	/*  Not found: do garbage collection  */
	pos = 0;                 /*  The position after compaction  */
	pos2 = 0;                /*  The position of the next valid string  */
	pos3 = gStringFreeRoot;  /*  The position of the next free space  */
	while (pos2 < gStringEnd - gStringBase) {
		if (pos2 == pos3) {
			/*  The present position is free; we need to skip to the next valid string  */
			pos3 = s_bs_get_free_space_info(pos2, &size);
			pos2 += size;
			continue;
		}
		p = gStringBasePtr + pos2 + 1;
		while (*p++ != 0);
		len2 = (p - gStringBasePtr) - pos2;  /*  Length of the string plus 2 (refcount and NUL)  */
		if (pos2 != pos) {
			/*  Copy pos2...pos2+len to pos, and update the string cell  */
			memmove(gStringBasePtr + pos, gStringBasePtr + pos2, len2);
			for (i = gStringCellsBase; i < gStringCellsEnd; i += sizeof(Off_t)) {
				if (*((Off_t *)(gMemoryPtr + i)) == pos2 + 1) {
					*((Off_t *)(gMemoryPtr + i)) = pos + 1;
					break;
				}
			}
		}
		/*  Skip the last unused NUL bytes  */
		while (p < gMemoryPtr + gStringEnd && *p == 0)
			p++;
		pos2 = p - gStringBasePtr;
		pos += len2;
	}

	/*  At this point, (gStringBase+pos)...gStringEnd is the free memory  */
	len2 = gStringEnd - gStringBase - pos;
	
	if (len2 < len || len2 < sizeof(Off_t) + 1) {
		/*  The free space is too small  */
		/*  Expand block  */
		if (bs_realloc_block(MEM_STRING, (gStringEnd - gStringBase) * 2) < 0)
			return kInvalidOff;  /*  Cannot allocate the free space  */
		len2 = gStringEnd - gStringBase - pos;
	}

	gStringFreeRoot = pos;
	s_bs_put_free_space_info(pos, len2, kInvalidOff);
	if (len2 >= len) {
		pos2 = kInvalidOff;
		pos3 = kInvalidOff;
		size = len2;
	}
	
found:
	/*  Find an unused cell  */
	for (i = gStringCellsBase; i < gStringCellsEnd; i += sizeof(Off_t)) {
		if (*((Off_t *)(gMemoryPtr + i)) == kInvalidOff) {
			*((Off_t *)(gMemoryPtr + i)) = pos + 1;
			break;
		}
	}
	if (i >= gStringCellsEnd) {
		/*  Expand block  */
		int j;
		if (bs_realloc_block(MEM_STRING_CELLS, (gStringCellsEnd - gStringCellsBase) * 2) < 0)
			return kInvalidOff;  /*  Cannot allocate the free space  */
		for (j = i; j < gStringCellsEnd; j += sizeof(Off_t))
			*((Off_t *)(gMemoryPtr + j)) = kInvalidOff;
		*((Off_t *)(gMemoryPtr + i)) = pos + 1;
	}
	
	if (size >= len + sizeof(Off_t) + 1) {
		/*  The remaining space is registered as a free space  */
		pos += len;
		if (pos3 == kInvalidOff)
			gStringFreeRoot = pos;
		else
			s_bs_put_free_space_info(pos3, 0, pos);
		s_bs_put_free_space_info(pos, size - len, pos2);
		pos -= len;
		memset(gStringBasePtr + pos, 0, len);
	} else {
		/*  This block is wholly used as a string  */
		if (pos3 == kInvalidOff)
			gStringFreeRoot = pos2;
		else
			s_bs_put_free_space_info(pos3, 0, pos2);
		memset(gStringBasePtr + pos, 0, size);
	}
	gStringBasePtr[pos] = 1;  /*  Reset reference count  */
	return i - gStringCellsBase;
}

Off_t
bs_new_runtime_string(const char *cs, int len)
{
	Off_t str;
	int len2;
	if (len == -1)
		len = strlen(cs);
	else if (cs != NULL) {
		len2 = strlen(cs);
		if (len > len2)
			len = len2;
	}
	str = bs_allocate_runtime_string(len);
	if (str != kInvalidOff && cs != NULL) {
		char *p = (char *)gStringBasePtr + *((Off_t *)(gStringCellsBasePtr + str));
		strncpy(p, cs, len);
		p[len] = 0;
	}
	return str;
}

Off_t
bs_retain_string(Off_t str)
{
	if ((str & kMSBOff) == 0) {
		Off_t strp = *((Off_t *)(gStringCellsBasePtr + str));
		if (gStringBasePtr[strp - 1] == 255) {
			/*  Allocate a new copy  */
			Int len = strlen((char *)gStringBasePtr + strp);
			Off_t str2 = bs_allocate_runtime_string(len);
			Off_t strp2;
			if (str2 == kInvalidOff)
				return str2;  /*  Out of memory  */
			strp2 = *((Off_t *)(gStringCellsBasePtr + str2));
			strcpy((char *)gStringBasePtr + strp2, (char *)gStringBasePtr + strp);
			return str2;
		}
		gStringBasePtr[strp - 1]++;
	}
	return str;
}

Off_t
bs_release_string(Off_t str)
{
	Off_t pos;
	if ((str & kMSBOff) != 0)
		return 0;
	pos = *((Off_t *)(gStringCellsBasePtr + str)) - 1;
	if (--gStringBasePtr[pos] == 0) {
		Off_t fpos, fpos2;
		u_int32_t size, size2;
		/*  The string is released; free the cell  */
		*((Off_t *)(gStringCellsBasePtr + str)) = kInvalidOff;
		str = pos + 1;
		while (gStringBasePtr[str] != 0)
			str++;
		while (str < gStringEnd - gStringBase && gStringBasePtr[str] == 0)
			str++;
		str -= pos;
		/*  Look for the position to insert the new free block  */
		fpos = gStringFreeRoot;
		fpos2 = kInvalidOff;
		size2 = 0;
		while (fpos != kInvalidOff && fpos < pos) {
			fpos2 = fpos;
			fpos = s_bs_get_free_space_info(fpos2, &size2);
		}
		if (fpos2 != kInvalidOff) {
			if (fpos2 + size2 == pos) {
				/*  Coerce with the last block  */
				size2 += str;
				s_bs_put_free_space_info(fpos2, size2, fpos);
			} else {
				s_bs_put_free_space_info(fpos2, size2, pos);
				s_bs_put_free_space_info(pos, str, fpos);
			}
		} else {
			s_bs_put_free_space_info(pos, str, fpos);
			gStringFreeRoot = pos;
			fpos2 = pos;
			size2 = str;
		}
		if (fpos2 != kInvalidOff && fpos2 + size2 == fpos) {
			/*  Coerce with the next block  */
			pos = s_bs_get_free_space_info(fpos, &size);
			size2 += size;
			s_bs_put_free_space_info(fpos2, size2, pos);
		}
	}
	return 0;
}

char *
bs_get_string_ptr(Off_t str)
{
	if ((str & kMSBOff) != 0) {
		str &= ~kMSBOff;
		if (str >= 0 && str < gConstStringEnd - gConstStringBase)
			return (char *)gConstStringBasePtr + str;
		else return NULL;
	} else {
		if (str >= 0 && str < gStringCellsEnd - gStringCellsBase)
			return (char *)gStringBasePtr + *((Off_t *)(gStringCellsBasePtr + str));
		else return NULL;
	}
}

#if 0
#pragma mark ====== Handling compressed integer ======
#endif

/*  pos is offset from gMemoryPtr  */
Off_t
bs_put_compressed_integer(Off_t pos, u_int32_t num)
{
	u_int8_t *p = gMemoryPtr + pos;
	while (num >= 0x80) {
		*p++ = 0x80 + (num & 0x7f);
		num >>= 7;
	}
	*p++ = num;
	return p - gMemoryPtr;
}

Off_t
bs_get_compressed_integer(Off_t pos, u_int32_t *nump)
{
	u_int32_t num = 0;
	u_int8_t *p = gMemoryPtr + pos;
	int n = 0;
	do {
		num += (u_int32_t)(*p & 0x7f) << n;
		n += 7;
	} while (*p++ >= 0x80);
	*nump = num;
	return p - gMemoryPtr;
}


#if 0
#pragma mark ====== For Debug ======
#endif

#if 0

#define STRING_TEST_SIZE 4

#include <stdio.h>
#include <stdlib.h>

void
bs_dump(void)
{
	int i, fcount;
	Off_t pos, fpos, pos2;
	u_int32_t size, tsize;
	debug_printf("--------------\n");
	for (i = gStringCellsBase; i < gStringCellsEnd; i += sizeof(Off_t)) {
		pos = *((Off_t *)(gMemoryPtr + i));
		if (pos == kInvalidOff)
			debug_printf("* ");
		else debug_printf("@%d ", (int)((i - gStringCellsBase) / sizeof(Off_t) + 1));
	}
	debug_printf("\n");
	pos = 0;
	fpos = gStringFreeRoot;
	fcount = 0;
	tsize = 0;
	while (pos < gStringEnd - gStringBase) {
		if (fpos == pos) {
			fpos = s_bs_get_free_space_info(fpos, &size);
			debug_printf("%d: Free #%d (%d bytes)\n", pos, ++fcount, size);
			pos += size;
			tsize += size;
		} else {
			for (i = (gStringCellsEnd - gStringCellsBase) / sizeof(Off_t) - 1; i >= 0; i--) {
				if (((Off_t *)gStringCellsBasePtr)[i] == pos + 1)
					break;
			}
			pos2 = pos;
			while (gStringBasePtr[pos2] != 0) pos2++;
			while (pos2 < gStringEnd - gStringBase && gStringBasePtr[pos2] == 0) pos2++;
			size = pos2 - pos;
			debug_printf("%d: Str[@%d], \"%s\" (%d refs, %d bytes)\n",
				   pos, i + 1,
				   (char *)gStringBasePtr + pos + 1, gStringBasePtr[pos], size);
			tsize += size;
			pos = pos2;
		}
	}
	debug_printf("Total %d bytes out of %d bytes\n", tsize,
		   (gStringEnd - gStringBase));
}

void
bs_test(void)
{
	char buf[1024];
	int n;
	bs_init_memory();
	gStringCellsEnd = gStringCellsBase + sizeof(Off_t) * STRING_TEST_SIZE;
	gStringEnd = gStringBase + 32;
	for (n = 0; n <= MEM_END; n++)
		gMemoryPtrs[n] = gMemoryPtr + gMemoryOffsets[n];
	bs_init_runtime_string();
	bs_dump();
	while (fgets(buf, sizeof buf, stdin) != NULL) {
		buf[strlen(buf) - 1] = 0;  /*  Chop the last newline  */
		if (buf[0] == '!') {
			n = bs_new_runtime_string(buf + 1, -1);
			debug_printf("Return value: %d: %s\n", n, bs_get_string_ptr(n));
		}
		else if (buf[0] >= '0' && buf[0] <= '9')
			bs_retain_string((atoi(buf) - 1) * sizeof(Off_t));
		else if (buf[0] == '-')
			bs_release_string((atoi(buf + 1) - 1) * sizeof(Off_t));
		bs_dump();
	}
}

void
bs_const_dump(void)
{
	int len;
	Off_t pos;
	debug_printf("--------------\n");
	for (pos = gConstStringBase; pos < gConstStringTop; pos += len) {
		len = strlen((char *)gMemoryPtr + pos) + 1;
		debug_printf("%d: \"%s\" (%d bytes)\n", pos - gConstStringBase, (char *)gMemoryPtr + pos, len);
	}
	debug_printf("%d bytes free\n", gConstStringEnd - pos);
}

void
bs_const_test(void)
{
	char buf[1024];
	int n;
	bs_init_memory();
	gConstStringEnd = gConstStringBase + 32;
	for (n = 0; n <= MEM_END; n++)
		gMemoryPtrs[n] = gMemoryPtr + gMemoryOffsets[n];
	bs_const_dump();
	while (fgets(buf, sizeof buf, stdin) != NULL) {
		buf[strlen(buf) - 1] = 0;  /*  Chop the last newline  */
		if (buf[0] == '!') {
			n = bs_new_literal_string(buf + 1, -1);
			debug_printf("Return value: %d: %s\n", n & ~kMSBOff, bs_get_string_ptr(n));
		}
		bs_const_dump();
	}
}

int
main(int argc, const char **argv)
{
	bs_const_test();
	return 0;
}

#endif
