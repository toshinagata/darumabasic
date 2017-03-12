/*
 *  main.c
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
#include <unistd.h>  /*  chdir()  */
#include <ctype.h>

#include "daruma.h"
#include "screen.h"
#include "gencode.h"
#include "transmessage.h"

/*  Base directory  */
char bs_basedir[MAXPATHLEN];

/*  Current file name (relative to the base directory, or absolute path)  */
char bs_filename[MAXPATHLEN];

/*  For debug  */
#if defined(ENABLE_DEBUG_PRINTF)
int
debug_printf(const char *fmt,...)
{
	int n = 0;
	va_list ap;
	char buf[1024];
	va_start(ap, fmt);
	n = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return n;
}
#endif

/*  Clear all program/compile/runtime storage  */
/*  (i.e. Everything except for the source program)  */
int
bs_new(void)
{
	/*  Clear source program  */
	gSourceTop = 0;
	
	/*  Clear parser information and runtime memory  */
	bs_init_parser_memory();
	
	return 0;
}

int
bs_run(const u_int8_t *ptr, int new_run, int direct_mode)
{
	int n;
	Off_t save_vmcode, save_vmdata, save_vmaddress, save_const_string;
	Off_t save_var_info, save_dim_info, save_label_info, save_func_info, save_loop_info;

	if (new_run)
		bs_init_parser_memory();

	save_vmcode = gVMCodeTop;
	save_vmdata = gVMDataTop;
	save_vmaddress = gVMAddressTop;
	save_const_string = gConstStringTop;
	save_var_info = gVarInfoTop;
	save_dim_info = gDimInfoTop;
	save_label_info = gLabelInfoTop;
	save_func_info = gFuncInfoTop;
	save_loop_info = gLoopInfoTop;
	
	bs_start_parser_on_memory(ptr, new_run);

	gRunMode = BS_RUNMODE_PARSE;
	n = yyparse();
	gRunMode = BS_RUNMODE_NONE;
	
#if 1
	/*  For debug  */
	bs_dump_vmcode(save_vmcode, 1000);
#endif
	
 	if (n != 0) {
	/*	bs_puts(MSG_(BS_M_COMPILE_ERROR)); */
		gVMCodeTop = save_vmcode;
		gVMDataTop = save_vmdata;
		gVMAddressTop = save_vmaddress;
		gConstStringTop = save_const_string;
		gVarInfoTop = save_var_info;
		gDimInfoTop = save_dim_info;
		gLabelInfoTop = save_label_info;
		gFuncInfoTop = save_func_info;
		gLoopInfoTop = save_loop_info;
		return -1; /*  Compile error  */
	}
	
	n = bs_execinit(save_vmcode, new_run);
	if (n == 0) {
		gRunMode = (direct_mode ? BS_RUNMODE_DIRECT : BS_RUNMODE_BATCH);
		n = bs_execcode();
		gRunMode = BS_RUNMODE_NONE;
		my_suppress_update = 0;
	}
	
	if (n != 0) {
		debug_printf("Runtime error occurred.\n");
	}
	
	if (!new_run) {
		/*  The compiled code will be discarded, so we also discard
		    internal info that is dependent on the compiled code  */
		gVMCodeTop = save_vmcode;
		gVMDataTop = save_vmdata;
		gVMAddressTop = save_vmaddress;
		gLabelInfoTop = save_label_info;
		gFuncInfoTop = save_func_info;
		gLoopInfoTop = save_loop_info;
	}
	
	return n; /* Positive number: runtime error */
}

int
bs_welcome(void)
{
	/*  Welcome message  */
	bs_tcolor(RGBFLOAT(1, 1, 1));
	bs_puts("---------------------------------------\n");
	bs_puts(" Daruma (だるま) BASIC, Ver. 1.0pre3\n");
	bs_puts(" Copyright (C) 2016-2017 Toshi Nagata\n");
	bs_puts("---------------------------------------\n");
	bs_puts(MSG_(BS_M_START_PROGRAM));
	return 0;
}

static char s[1024];

int
bs_runloop(void)
{
	FILE *fp;
	int n;
	int cont_flag = 0;
	char *p;
	u_int64_t ut;
	
	bs_init_screen();
	bs_show_darumalogo();
	bs_uptime(1);  /*  Set the 'startup' time  */
	
	bs_init_memory();
	bs_read_fontdata("fontdata.bin");
	
	if ((p = getenv("DARUMA_BASIC_DIR")) != NULL && *p != 0) {
		n = chdir(p);
	} else n = -1;
	if (n < 0) 
		chdir("basic");

	getcwd(bs_basedir, sizeof(bs_basedir));

	bs_new();

	ut = bs_uptime(0);
	if (ut < 1000000) {
		usleep(1000000 - ut);
	}
	
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	bs_clear_box(0, 0, my_width, my_height);

	bs_welcome();

	/*  Internal error check  */
	if (C_STOP >= 255) {
		bs_puts("Internal Error: too many VM opecodes are defined");
		bs_update_screen();
		usleep(2000000);
		return -1;
	}
	
	n = -1;
	fp = fopen("autorun.bas", "r");
	if (fp != NULL) {
		fclose(fp);
		bs_puts_format(MSG_(BS_M_RUNNING), "autorun.bas");
		n = bs_load_file("autorun.bas");
		if (n == 0) {
			n = bs_run(gSourceBasePtr, 1, 0);  /*  Run  */
			if (n < 0)
				cont_flag = 0;  /*  Compile error  */
			else
				cont_flag = 1;  /*  Normal termination or runtime error  */
		}
	} else n = -1;

	while (1) {
	
		bs_puts(">> ");
		if (bs_getline(s, sizeof s - 1) > 0) {
			int n1;
			for (n = 0; s[n] > 0 && s[n] <= ' '; n++);
			if (s[n] == 0)
				continue;
			for (n1 = n; isalpha(s[n1]); n1++);
			n1 -= n;
			if (n1 > 0) {
				/*  Check for the 'direct' commands  */
				if (strncasecmp(s + n, "RUN", n1) == 0) {
					cont_flag = 0;
					n = bs_run(gSourceBasePtr, 1, 0);
					if (n < 0)
						cont_flag = 0;  /*  Compile error  */
					else
						cont_flag = 1;  /*  Normal termination of runtime error  */
					continue;
				} else if (strncasecmp(s + n, "NEW", n1) == 0) {
					/*  Initialize the program and lua engine  */
					if (gSourceTop > 0) {
						bs_puts(MSG_(BS_M_PROG_DELETED_YN));
						if (bs_getline(s, sizeof s) > 0 && (s[0] == 'y' || s[0] == 'Y'))
							bs_new();
					}
					continue;
				} else if (strncasecmp(s + n, "EDIT", n1) == 0) {
					bs_edit();
					continue;
				} else if (strncasecmp(s + n, "EXIT", n1) == 0) {
					bs_puts(MSG_(BS_M_EXIT_IN_DIRECT));
					continue;
				} else if (strncasecmp(s + n, "QUIT", n1) == 0) {
					u_int64_t us1, us2;
					if (gSourceTop > 0 && gSourceTouched) {
						bs_puts(MSG_(BS_M_PROG_SAVE_YN));
						if (bs_getline(s, sizeof s) <= 0 || (s[0] != 'y' && s[0] != 'Y'))
							continue;
					}
					bs_puts(MSG_(BS_M_FINISHED));
					bs_update_screen();
					usleep(1000000);
					us1 = bs_uptime(0);
					for (n = 20; n >= 0; n--) {
						bs_fadeout(n);
						us2 = bs_uptime(0);
						if (us1 + 50000 > us2 + 20)
							usleep(us1 + 50000 - us2);
						us1 = us2;
					}
					break;
				}
			}
			strcat(s + n, "\n");
			n = bs_run((u_int8_t *)s + n, !cont_flag, 1);
			if (cont_flag == 0 && n >= 0)
				cont_flag = 1;
		}
	}
	
	return n;
}
