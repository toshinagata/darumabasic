/*
 *  raspi_main.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 2016/02/07.
 *  Copyright 2016 Toshi Nagata.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "daruma.h"
#include "screen.h"

#ifndef __BAREMETAL__

#include <fcntl.h>
#include <termios.h>

static struct termios s_SaveTermIos;
static struct termios s_RawTermIos;

int
tty_puts(const char *s)
{
	return fputs(s, stdout);
}

int
tty_putc(int c)
{
	return fputc(c, stdout);
}

int
tty_getch(void)
{
	u_int8_t c;
	int n;
	n = read(STDIN_FILENO, &c, 1);
	if (n > 0)
		return c;
	else return -1;	
}

void
set_raw_mode(void)
{
	tcgetattr(STDIN_FILENO, &s_SaveTermIos);
	s_RawTermIos = s_SaveTermIos;
	cfmakeraw(&s_RawTermIos);
	s_RawTermIos.c_cc[VMIN] = 1;
	s_RawTermIos.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, 0, &s_RawTermIos);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	setvbuf(stdout, NULL, _IONBF, 0);
	tty_puts("\x1b[H");     /*  Set cursor home  */
	tty_puts("\x1b[J");     /*  Erase to end of screen  */
	tty_puts("\x1b[?25l");  /*  Hide cursor  */
}

void
unset_raw_mode(void)
{
	char buf[20];
	tcsetattr(STDIN_FILENO, 0, &s_SaveTermIos);
	snprintf(buf, sizeof buf, "\x1b[%d;1H", my_height / 16);  /*  Cursor down full screen  */
	tty_puts(buf);
	tty_puts("\x1b[?25h");  /*  Show cursor  */
}

#endif

#ifndef __circle__
int
main(int argc, const char **argv)
{
	char *p;

	/*  Set current directory to the same direcory as this executable  */
	p = strrchr(argv[0], '/');
	if (p != NULL) {
		int n = p - argv[0];
		char *pp = (char *)bs_malloc(n + 1);
		strncpy(pp, argv[0], n);
		pp[n] = 0;
		chdir(pp);
		bs_free(pp);
	}
	
	my_graphic_mode = 1;

#ifndef __CONSOLE__
	if (argc > 1 && strcmp(argv[1], "-t") == 0) {
		argc--;
		argv++;
		my_console = BS_CONSOLE_TTY;
	}
#else
	my_console = BS_CONSOLE_TTY;
#endif
	
	if (argc > 1 && strcmp(argv[1], "-0") == 0) {
		argc--;
		argv++;
		my_graphic_mode = 0;
	}
	
#ifdef __BAREMETAL__
	{
		extern int baremetal_init(void);
		baremetal_init();
	}
#else
	set_raw_mode();
	atexit(unset_raw_mode);
#endif
	
	bs_runloop();
	return 0;
}
#endif
