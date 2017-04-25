/*
 *  screen.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 16/01/09.
 *  Copyright 2016 Toshi Nagata. All rights reserved.
 *
 */

#include "screen.h"
#include "graph.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

/*  Text output destination  */
u_int8_t my_console;

/*  Current screen size  */
int16_t my_width, my_height;

/*  Frame buffer size (may be different from my_width, my_height)  */
int16_t my_fb_width, my_fb_height;

/*  Internal bitmap buffer  */
/* pixel_t *my_textscreen;
pixel_t *my_graphscreen;
pixel_t *my_composedscreen; */

/*  Text cursor  */
int16_t my_cursor_x, my_cursor_y;
int16_t my_cursor_xofs, my_cursor_yofs;

/*  Show cursor?  */
u_int8_t my_show_cursor;

/*  Suppress automatic screen update  */
u_int8_t my_suppress_update;

/*  Exchange the destination buffer, i.e. text drawing to graphic buffer,
    and graphic drawing to text buffer  */
/* u_int8_t my_exchange_buffer; */

int16_t my_max_x, my_max_y;

/*  Tab base and width  */
int16_t my_tab_base, my_tab_width;

/*  Graphic mode  */
/*  0: normal, 1: 2x2 expansion  */
int16_t my_graphic_mode;

/*  Origin of the visible part of the screen (expansion mode)  */
/*int16_t my_origin_x, my_origin_y; */

/*  Graphic patterns  */
/*pixel_t *my_patbuffer;
u_int32_t my_patbuffer_size;
u_int32_t my_patbuffer_count;
LPattern my_patterns[NUMBER_OF_PATTERNS]; */

static pixel_t s_tcolor = RGBFLOAT(1, 1, 1);
static pixel_t s_bgcolor = 0;

static pixel_t s_palette[16];
static pixel_t s_initial_palette[16] = {
	0, RGBFLOAT(0, 0, 1), RGBFLOAT(0, 1, 0), RGBFLOAT(0, 1, 1),
	RGBFLOAT(1, 0, 0), RGBFLOAT(1, 0, 1), RGBFLOAT(1, 1, 0), RGBFLOAT(1, 1, 1),
	RGBFLOAT(0.5, 0.5, 0.5), RGBFLOAT(0, 0, 0.67), RGBFLOAT(0, 0.67, 0), RGBFLOAT(0, 0.67, 0.67),
	RGBFLOAT(0.67, 0, 0), RGBFLOAT(0.67, 0, 0.67), RGBFLOAT(0.67, 0.67, 0), RGBFLOAT(0.67, 0.67, 0.67)
};

#if !__CONSOLE__
#if EMBED_FONTDATA
#define FONTDATA_BIN _binary____common_fontdata_bin_start
extern char FONTDATA_BIN[];
u_int16_t *gConvTable;
u_int8_t *gFontData;
u_int8_t *gKanjiData;
#else
u_int16_t gConvTable[65536];
u_int8_t gFontData[16*188];
u_int8_t gKanjiData[32*11844];
#endif
#endif

int
bs_fread(u_int8_t *pos, size_t size, size_t nitems, FILE *fp)
{
	int rsize = 0, n;
	size *= nitems;
	while (rsize < size) {
		n = size - rsize;
		if (n > 4096)
			n = 4096;
		if (fread(pos + rsize, 1, n, fp) < n) {
			return -1;
		}
		rsize += n;
	}
	return size;
}

/*  Read character data  */
int
bs_read_fontdata(const char *path)
{
#ifndef __CONSOLE__
	static u_int8_t s_done = 0;
	FILE *fp;
	u_int8_t buf[4];
	int uc;
	int convsize, fontsize, kanjisize;

	if (s_done)
		return 0;

	fp = fopen(path, "r");

	if (fp == NULL) {
#if EMBED_FONTDATA
		/*  Data is already at FONTDATA_BIN  */
		convsize = *((u_int32_t *)FONTDATA_BIN);
		fontsize = *((u_int32_t *)(FONTDATA_BIN + 4));
		kanjisize = *((u_int32_t *)(FONTDATA_BIN + 8));
		gConvTable = (u_int16_t *)(FONTDATA_BIN + 12);
		gFontData = (u_int8_t *)(FONTDATA_BIN + 12 + convsize*2);
		gKanjiData = (u_int8_t *)(FONTDATA_BIN + 12 + convsize*2 + fontsize);
		return 0;
#else
	//	log_printf("Cannot read font data %s\n", path);
		return 1;
#endif
	}
	
	fread(buf, 1, 4, fp);
	convsize = (u_int32_t)buf[0] + 256 * ((u_int32_t)buf[1] + 256 * ((u_int32_t)buf[2] + 256 * (u_int32_t)buf[3]));	
	fread(buf, 1, 4, fp);
	fontsize = (u_int32_t)buf[0] + 256 * ((u_int32_t)buf[1] + 256 * ((u_int32_t)buf[2] + 256 * (u_int32_t)buf[3]));	
	fread(buf, 1, 4, fp);
	kanjisize = (u_int32_t)buf[0] + 256 * ((u_int32_t)buf[1] + 256 * ((u_int32_t)buf[2] + 256 * (u_int32_t)buf[3]));	
	
#if EMBED_FONTDATA
	gConvTable = (u_int16_t *)malloc(convsize*2 + fontsize + kanjisize);
	if (gConvTable == NULL) {
		log_printf("Cannot allocate memory for font data\n");
		return 1;
	}
	gFontData = (u_int8_t *)(gConvTable + convsize);
	gKanjiData = gFontData + fontsize;
#endif
	
	fread(gConvTable, 2, convsize, fp);
	for (uc = 0; uc < 65536; uc++) {
		/*  Convert from little endian  */
		u_int16_t si;
		si = ((u_int8_t *)&gConvTable[uc])[0];
		si += 256 * (u_int16_t)((u_int8_t *)&gConvTable[uc])[1];
	}
	bs_fread(gFontData, 1, fontsize, fp);
	bs_fread(gKanjiData, 1, kanjisize, fp);
	
	fclose(fp);

	s_done = 1;
	
#endif  /*  __CONSOLE__  */
	return 0;
}

/*  Scroll the buffer. bufindex = 0: text buffer, 1: graphic buffer  */
/*  The y coordinates are graphic coordinates (i.e. up is positive)  */
void
bs_scroll(int16_t bufindex, int16_t x, int16_t y, int16_t width, int16_t height, int16_t dx, int16_t dy)
{
	if (my_console == BS_CONSOLE_TTY) {
		int cx, cy, y1, y2;
		char buf[16];
		if (bufindex != 0)
			return;
		cx = my_cursor_x;
		cy = my_cursor_y;
		/*  Only support vertical scrolling  */
		/*  Set scroll area  */
		y1 = (my_height - y - height) / 16 + 1 - my_cursor_yofs;
		y2 = y1 + height / 16 - 1;
		snprintf(buf, sizeof buf, "\x1b[%d;%dr", y1, y2);
		tty_puts(buf);
		dy = dy / 16;
		if (dy < 0) {
			/*  Down scroll  */
			dy = -dy;
			bs_locate(0, 0);
			while (--dy >= 0)
				tty_puts("\x1bM");
		} else if (dy > 0) {
			/*  Up scroll  */
			bs_locate(0, (y + height) / 16 - 1);
			while (--dy >= 0)
				tty_puts("\x1b" "D");
		}
		/*  Reset scroll area  */
		snprintf(buf, sizeof buf, "\x1b[%d;%dr", 1, my_height / 16);
		tty_puts(buf);
		bs_locate(cx, cy);
		return;
	}
#if !__CONSOLE__
	{
		int x1, y1, w1, h1;
		bs_select_active_buffer(TEXT_ACTIVE);
		if (dx > 0) {
			w1 = width - dx;
			x1 = x;
		} else {
			w1 = width + dx;
			x1 = x + dx;
		}
		if (dy > 0) {
			h1 = height - dy;
			y1 = y;
		} else {
			h1 = height + dy;
			y1 = y - dy;
		}
		if (w1 > 0 && h1 > 0) {
			bs_copy_pixels(x1 + dx, y1 + dy, x1, y1, w1, h1);
		}
		if (dx > 0) {
			bs_clear_box(x, y, dx, height);
		} else if (dx < 0) {
			bs_clear_box(x + w1, y, -dx, height);
		}
		if (dy > 0) {
			bs_clear_box(x, y, width, dy);
		} else if (dy < 0) {
			bs_clear_box(x, y + h1, width, -dy);
		}
	}
	
#endif /* !__CONSOLE__ */
}

static void
s_bs_putonechar(int16_t x, int16_t y, u_int16_t ch, pixel_t fcolor, pixel_t bcolor)
{
#if !__CONSOLE__
	u_int8_t *f;
	static pixel_t buf[256];
	pixel_t *p = buf;
	u_int32_t idx = gConvTable[ch];
	int i, j, width = 8;
	if (ch == 32) {
		for (i = 0; i < 16; i++) {
			for (j = 0; j < 8; j++) {
				*p++ = bcolor;
			}
		}
	} else if (idx == 0xffff)
		return;
	else if (idx < 188) {
		f = gFontData + idx * 16;
		for (i = 0; i < 16; i++) {
			for (j = 0; j < 8; j++) {
				u_int8_t b = (*f & (1 << j));
				*p++ = (b != 0 ? fcolor : bcolor);
			}
			f++;
		}
	} else if (idx < 12032) {
		f = gKanjiData + (idx - 188) * 32;
		for (i = 0; i < 16; i++) {
			u_int16_t u = *f++;
			u += (u_int16_t)*f++ * 256;
			for (j = 0; j < 16; j++) {
				u_int16_t b = (u & (1 << j));
				*p++ = (b != 0 ? fcolor : bcolor);
			}
		}
		width = 16;
	}
	bs_put_pattern(buf, x, y, width, 16);
#endif  /* !__CONSOLE__ */
}

/*  Erase to end of line  */
void
bs_erase_to_eol(void)
{
	if (my_console == BS_CONSOLE_TTY) {
		tty_puts("\x1b[K");
		return;
	}
#if !__CONSOLE__
	{
	/*	int width = (my_max_x - my_cursor_x) * 8 * sizeof(pixel_t); */
		bs_select_active_buffer(TEXT_ACTIVE);
		bs_clear_box((my_cursor_xofs + my_cursor_x) * 8, my_height - (my_cursor_yofs + my_cursor_y + 1) * 16, my_width - my_cursor_x * 8, 16);
	}
#endif
}

/*  Clear screen  */
void
bs_cls(void)
{
	if (my_console == BS_CONSOLE_TTY) {
		tty_puts("\x1b[2J");
		return;
	}
#if !__CONSOLE__
	bs_select_active_buffer(TEXT_ACTIVE);
	bs_clear_box(0, 0, my_width, my_height);
#endif
}

/*  Character width  */
int
bs_character_width(u_int16_t uc)
{
	if (uc < 0x80 || (uc >= 0xff60 && uc <= 0xff9f))
		return 1;
	else return 2;
}

u_int16_t
bs_utf8_to_utf16(const char *s, char **outpos)
{
	u_int8_t c;
	u_int16_t uc;
	c = *s++;
	if (c < 0x80) {
		uc = c;
	} else if (c >= 0xc2 && c <= 0xdf) {
		uc = (((u_int16_t)c & 0x1f) << 6) + (((u_int16_t)*s++) & 0x3f);
	} else if (c >= 0xe0 && c <= 0xef) {
		uc = (((u_int16_t)c & 0x0f) << 12) + ((((u_int16_t)*s++) & 0x3f) << 6);
		uc += ((u_int16_t)*s++) & 0x3f;
	} else uc = 32;  /*  Out of range  */
	if (outpos != NULL)
		*outpos = (char *)s;
	return uc;
}

/*  The output buffer s[] must have at least three bytes space  */
char *
bs_utf16_to_utf8(u_int16_t uc, char *s)
{
	if (uc < 0x80) {
		*s++ = uc;
	} else if (uc < 0x0800) {
		*s++ = ((uc >> 6) & 31) | 0xc0;
		*s++ = (uc & 63) | 0x80;
	} else {
		*s++ = ((uc >> 12) & 15) | 0xe0;
		*s++ = ((uc >> 6) & 63) | 0x80;
		*s++ = (uc & 63) | 0x80;
	}
	*s = 0;
	return s;
}

/*  Draw a UTF-8 string on the graphic screen, starting at the designated position
    (x, y) is the lower-left corner of the string  */
void
bs_gputs(const char *s, int x, int y, pixel_t fcolor, pixel_t bcolor)
{
	u_int16_t uc;
	int x1, y1;
	x1 = x;
	y1 = y;
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	while (1) {
		uc = bs_utf8_to_utf16(s, (char **)&s);
		if (uc == 0)
			break;
		if (uc == '\n') {
			y -= 16;
			y1 = y;
			x1 = x;
			continue;
		}
		if (x1 >= 0 && x1 < my_width && y1 >= 0 && y1 < my_height)
			s_bs_putonechar(x1, y1, uc, fcolor, bcolor);
		x1 += bs_character_width(uc) * 8;
	}
}

/*  Draw a UTF-8 or UTF-16 string at the current cursor position  */
void
bs_puts_utf8or16(const char *s, int is_utf16)
{
	u_int16_t uc;
	u_int16_t *us = (u_int16_t *)s;
	int dx, cx, cy;
	int cx1, cx2, cy1, cy2;
	if (s == NULL)
		return;
	cx = my_cursor_x;
	cy = my_cursor_y;
	cx1 = cx2 = cx;
	cy1 = cy2 = cy;
	bs_select_active_buffer(TEXT_ACTIVE);
	while (1) {
		if (is_utf16)
			uc = *us++;
		else
			uc = bs_utf8_to_utf16(s, (char **)&s);
		if (uc == 0)
			break;
		if (uc == '\n') {
			cx = 0;
			cy++;
			if (cy == my_max_y) {
				/*  Scroll up  */
				bs_scroll(0, 0, 0, my_max_x * 8, my_max_y * 16, 0, 16);
				cy--;
			}
			dx = 0;
			goto cont;
		} else if (uc == '\t') {
			dx = (cx + my_tab_width - my_tab_base) / my_tab_width * my_tab_width + my_tab_base - cx;
			cx += dx;
			if (cx >= my_max_x) {
				cx = 0;
				cy++;
				if (cy == my_max_y) {
					bs_scroll(0, 0, 0, my_max_x * 8, my_max_y * 16, 0, 16);
					cy--;
				}
			}
			goto cont;
		}
		dx = bs_character_width(uc);
		if (dx == 2 && cx == my_max_x - 1) {
			cx = 0;
			cy++;
			if (cy == my_max_y) {
				bs_scroll(0, 0, 0, my_max_x * 8, my_max_y * 16, 0, 16);
				cy--;
			}
		}
		if (my_console == BS_CONSOLE_TTY) {
			char utf8[8];
			bs_utf16_to_utf8(uc, utf8);
			tty_puts(utf8);
		} else s_bs_putonechar((cx + my_cursor_xofs) * 8, my_height - (cy + my_cursor_yofs + 1) * 16, uc, s_tcolor, s_bgcolor);
		cx += dx;
		if (cx < cx1)
			cx1 = cx;
		if (cx > cx2)
			cx2 = cx;
		if (cy < cy1)
			cy1 = cy;
		if (cy > cy2)
			cy2 = cy;
		if (cx >= my_max_x) {
			cx = 0;
			cy++;
			if (cy == my_max_y) {
				bs_scroll(0, 0, 0, my_max_x * 8, my_max_y * 16, 0, 16);
				cy--;
			}
		}
	cont:
		if (dx >= 2 || cx == 0) {
			/*  relocate cursor  */
			bs_locate(cx, cy);
			if (my_console == BS_CONSOLE_TTY)
				bs_usleep(1000);
		}
	}
	my_cursor_x = cx;
	my_cursor_y = cy;
#if !__CONSOLE__
	if (my_console != BS_CONSOLE_TTY)
		bs_redraw((cx1 + my_cursor_xofs) * 8, my_height - (cy2 + my_cursor_yofs + 1) * 16, (cx2 - cx1) * 8, (cy2 - cy1 + 1) * 16);
#endif
}

void
bs_puts(const char *s)
{
	bs_puts_utf8or16(s, 0);
}

void
bs_puts_format(const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	bs_puts(buf);
}

void
bs_puts_utf16(const u_int16_t *s)
{
	bs_puts_utf8or16((const char *)s, 1);
}

void
bs_show_cursor(int flag)
{
	my_show_cursor = flag;
#if !__CONSOLE__
	bs_redraw((my_cursor_x + my_cursor_xofs) * 8, my_height - (my_cursor_y + my_cursor_yofs + 1) * 16, 8, 16);
#endif
}

int
bs_getline(char *buf, int size)
{
	u_int16_t *ubuf = (u_int16_t *)bs_malloc(sizeof(u_int16_t) * size);
	int usize = size;
	u_int16_t uc;
	char *s;
	int cx, cy, pt, len;
	int i, cx1, cy1, cx2, cy2, dx;
	memset(ubuf, 0, size * 2);
	cx = my_cursor_x;
	cy = my_cursor_y;
	pt = len = 0;
	while ((uc = bs_getch(1)) != 13) {
		if (uc == 3) {
			bs_free(ubuf);
			return -1;  /*  Interrupt  */
		}
		if (uc == 30 || uc == 1) {
			pt = 0;
		} else if (uc == 31 || uc == 5) {
			pt = len;
		} else if (uc == 28 || uc == 6) {
			if (pt < len)
				pt++;
		} else if (uc == 29 || uc == 2) {
			if (pt > 0)
				pt--;
		} else if (uc == 8 || uc == 127) {
			if (pt > 0) {
				memmove(ubuf + pt - 1, ubuf + pt, (len - pt + 1) * sizeof(u_int16_t));
				pt--;
				len--;
				ubuf[len] = 0;
			}
		} else {
			if (len < usize) {
				memmove(ubuf + pt + 1, ubuf + pt, (len - pt + 1) * sizeof(u_int16_t));
				ubuf[pt] = uc;
				pt++;
				len++;
			}
		}
		bs_locate(cx, cy);
		bs_puts_utf16(ubuf);
		bs_puts(" ");  /*  The cursor position  */
		bs_erase_to_eol();
		cx1 = cx2 = cx;
		cy1 = cy2 = cy;
		for (i = 0; i <= len; i++) {
			if (i == len)
				dx = 1;
			else {
				dx = bs_character_width(ubuf[i]);
			}
			if (dx == 2 && cx1 == my_max_x - 1) {
				cx1 = 0;
				cy1++;
			}
			if (i == pt) {
				cx2 = cx1;
				cy2 = cy1;
			}
			cx1 += dx;
			if (cx1 >= my_max_x) {
				cx1 = 0;
				cy1++;
			}
		}
		if (cy1 > my_cursor_y) {
			/*  Up scroll by dy lines  */
			int dy = cy1 - my_cursor_y;
			cy -= dy;
			cy2 -= dy;
		}
		bs_locate(cx2, cy2);
	}
	
	s = buf;
	cx = 0;
	for (i = 0; i < len; i++) {
		s = bs_utf16_to_utf8(ubuf[i], s);
		if (s - buf > size - 4) {
			cx = 1;
			break;  /*  The string is truncated  */
		}
	}
	*s = 0;
	bs_free(ubuf);
	bs_puts("\n");
	return s - buf;
}

/*  Get the key input.  */
/*  wait == 1: wait for input  */
/*  0: no wait  */
/*  -1: scan only  */
u_int16_t
bs_getch(int wait)
{
	static u_int8_t s_buf[8];
	static int s_bufcount = 0, s_bufindex = 0;
	
	int c;
	int timer;
	int waiting_for_esc = 0;
	u_int32_t usec;

	if (my_console == BS_CONSOLE_TTY && wait == 1)
		tty_puts("\x1b[?25h");  /*  Show cursor (vt100)  */
	
	usec = (wait == 1 ? 50000 : 0);
	
	if (my_console != BS_CONSOLE_TTY)
		bs_show_cursor(1);

	if (s_bufcount == 0) {
		/*  Read from input  */
		s_bufindex = 0;
		timer = 0;
		while (1) {
			c = bs_getch_with_timeout(usec);
		/*  TODO: call Japanese input method here  */
		/*	if (wait == 1) {
				extern int init_im(void);
				init_im();
			} */
			if (c > 0) {
				if (my_console != BS_CONSOLE_TTY)
					bs_show_cursor(0);
				s_buf[s_bufcount++] = c;
				if (s_bufcount == 1 && c == 0x1b) {
					/*  Start ESC sequence  */
					waiting_for_esc = 1;
					timer = 0;
					continue;
				} else if (s_bufcount == 2 && c == '[') {
					/*  Start CSI sequence  */
					waiting_for_esc = 0;
					continue;
				} else if (s_bufcount == 3) {
					s_bufcount = 0;
					if (c == 'A')
						return 0x1e;
					else if (c == 'B')
						return 0x1f;
					else if (c == 'C')
						return 0x1c;
					else if (c == 'D')
						return 0x1d;
					s_bufcount = 3;
					break;
				}
				break;
			}
			if (wait != 1)
				break;
			if (waiting_for_esc && timer == 2)
				break;  /*  Stop waiting for the subsequent character and return 0x1b  */
			if (my_console != BS_CONSOLE_TTY) {
				if (timer % 20 == 0)
					bs_show_cursor(1);
				else if (timer % 20 == 10)
					bs_show_cursor(0);
			}
			if (++timer == 20)
				timer = 0;
		}
	}
	
	if (my_console == BS_CONSOLE_TTY && wait == 1)
		tty_puts("\x1b[?25l");  /*  Hide cursor (vt100)  */
	
	/*  Return the waiting character if any  */
	if (s_bufcount > 0) {
		c = s_buf[s_bufindex];
		if (wait != -1) {
			s_bufindex = (s_bufindex + 1) % sizeof(s_buf);
			s_bufcount--;
		}
		return c;
	}
	return 0;
}

int
bs_locate(int x, int y)
{
	if (my_console == BS_CONSOLE_TTY) {
		char buf[12];
		snprintf(buf, sizeof buf, "\x1b[%d;%dH", y + 1, x + 1);
		tty_puts(buf);
		fflush(stdout);
		my_cursor_x = x;
		my_cursor_y = y;
		return 0;
	}
#if !__CONSOLE__
	if (x >= 0 && x < my_max_x && y >= 0 && y < my_max_y) {
		bs_redraw((my_cursor_x + my_cursor_xofs) * 8, my_height - (my_cursor_y + my_cursor_yofs + 1) * 16, 8, 16);
		my_cursor_x = x;
		my_cursor_y = y;
		bs_redraw((my_cursor_x + my_cursor_xofs) * 8, my_height - (my_cursor_y + my_cursor_yofs + 1) * 16, 8, 16);
	}
	return 0;
#endif
}

pixel_t
bs_tcolor(pixel_t col)
{
	pixel_t oldcol = s_tcolor;
	s_tcolor = col;
	if (my_console == BS_CONSOLE_TTY) {
		/*  Set text color to the nearest one  */
		int r, g, b;
		char buf[8];
#if BYTES_PER_PIXEL == 2
		if (col == 0xffff)
			col = 9;
		else {
			r = (col >> 11) & 0x1f;
			g = (col >> 5) & 0x3f;
			b = (col & 0x1f);
			col = (b >= 24 ? 1 : 0);
			col |= (g >= 48 ? 2 : 0);
			col |= (r >= 24 ? 4 : 0);
		}
#else
		if (col == 0xffffffff)
			col = 9;
		else {
			r = (col >> 16) & 0xff;
			g = (col >> 8) & 0xff;
			b = (col & 0xff);
			col = (b >= 128 ? 1 : 0);
			col |= (g >= 128 ? 2 : 0);
			col |= (r >= 128 ? 4 : 0);
		}
#endif			
		snprintf(buf, sizeof buf, "\x1b[%dm", 30 + (int)col);
		tty_puts(buf);
	}
	return oldcol;
}

pixel_t
bs_bgcolor(pixel_t col)
{
	pixel_t oldcol = s_bgcolor;
	s_bgcolor = col;
	if (my_console == BS_CONSOLE_TTY) {
		/*  Set text color to the nearest one  */
		int r, g, b;
		char buf[8];
		if (col == 0)
			col = 9;
		else {
#if BYTES_PER_PIXEL == 2
			r = (col >> 11) & 0x1f;
			g = (col >> 5) & 0x3f;
			b = (col & 0x1f);
			col = (b >= 24 ? 1 : 0);
			col |= (g >= 48 ? 2 : 0);
			col |= (r >= 24 ? 4 : 0);
#else
			r = (col >> 16) & 0xff;
			g = (col >> 8) & 0xff;
			b = (col & 0xff);
			col = (b >= 128 ? 1 : 0);
			col |= (g >= 128 ? 2 : 0);
			col |= (r >= 128 ? 4 : 0);
#endif			
		}
		snprintf(buf, sizeof buf, "\x1b[%dm", 40 + (int)col);
		tty_puts(buf);
	}
	return oldcol;
}

volatile u_int8_t g_init_screen_done = 0;

int
bs_init_screen(void) 
{
	int i;
	static u_int8_t s_init_screen_platform_done = 0;
	int save_graphic_mode = my_graphic_mode;

	my_graphic_mode = 0;
/*	my_origin_x = my_origin_y = 0; */
	my_tab_base = 0;
	my_tab_width = 4;
	
#if !__CONSOLE__
	if (s_init_screen_platform_done == 0) {
		bs_init_screen_platform();
		s_init_screen_platform_done = 1;
	}
#endif
	
	for (i = 0; i < 16; i++)
		s_palette[i] = s_initial_palette[i];
	
	/*  Allocate pattern buffer for (32x32)xNUMBER_OF_PATTERNS (=512 KB)  */
/*	my_patbuffer_size = 32*32*NUMBER_OF_PATTERNS;
	my_patbuffer_count = 0;
	if (my_patbuffer != NULL)
		bs_free(my_patbuffer);
	my_patbuffer = (pixel_t *)bs_calloc(sizeof(pixel_t), my_patbuffer_size);
	memset(my_patterns, 0, sizeof(my_patterns)); */

	bs_redraw(0, 0, my_width, my_height);
	
	if (save_graphic_mode != 0) {
		if (my_fb_width >= 640 && my_fb_width >= 400) {
			bs_gmode_platform(save_graphic_mode);
			my_graphic_mode = save_graphic_mode;
		}
	}
	
	if (my_console == BS_CONSOLE_TTY) {
		/*  Determine screen size if possible  */
		char buf[16];
		/*  Default values  */
		my_max_x = 80;
		my_max_y = 24;
		tty_puts("\x1b[18t");
		for (i = 0; i < 16; i++) {
			int ch, j;
			for (j = 100; j >= 0; j--) {
				ch = bs_getch(0);
				if (ch != 0)
					break;
				bs_usleep(10000);
			}
			if (j < 0) {
				buf[i] = 0;
				break;  /*  Timeout  */
			}
			buf[i] = ch;
			if (ch == 't')
				break;
		}
		if (i < 16 && buf[i] == 't') {
			char *p = buf + 4;
			my_max_y = strtol(p, &p, 0);
			p++;
			my_max_x = strtol(p, &p, 0);
		}
		my_width = 8 * my_max_x;
		my_height = 16 * my_max_y;	
	} else {
		my_max_x = my_width / 8;
		my_max_y = my_height / 16;
	}
	
	g_init_screen_done = 1;
	
	return 0;
}

#if 0
#pragma mark ====== BASIC Interface ======
#endif

/*  LOCATE x, y
 Set the text screen position to (x, y). 
 */
int
bs_builtin_locate(void)
{
	Int x, y;
	bs_get_next_int_arg(&x);
	bs_get_next_int_arg(&y);
	bs_locate(x, y);
	return 0;
}

/*  CLS
 Clear the text screen. */
int
bs_builtin_cls(void)
{
	bs_cls();
	my_cursor_x = my_cursor_y = 0;
	return 0;
}

/*  CLEARLINE
 Clear to the end of the line.  */
int
bs_builtin_clearline(void)
{
	bs_erase_to_eol();
	return 0;
}

/*  COLOR c1[,c2]
 Set the foreground color to c1, and background color to c2 (if given).
 c1 and c2 are given as the palette number (0-15) */
int
bs_builtin_color(void)
{
	Int c1, c2;
	bs_get_next_int_arg(&c1);
	s_tcolor = s_palette[c1 % 16];
	if (my_console == BS_CONSOLE_TTY)
		bs_tcolor(s_tcolor);
	if (bs_get_next_int_arg(&c2)) {
		s_bgcolor = s_palette[c2 % 16];
		if (my_console == BS_CONSOLE_TTY)
			bs_bgcolor(s_bgcolor);
	}
	return 0;
}

/*  TCOLOR c1[,c2]
 Set the foreground color to c1, and background color to c2 (if given).
 c1 and c2 are given as the rgb value */
int
bs_builtin_tcolor(void)
{
	Int c1, c2;
	bs_get_next_int_arg(&c1);
	s_tcolor = c1;
	if (my_console == BS_CONSOLE_TTY)
		bs_tcolor(s_tcolor);
	if (bs_get_next_int_arg(&c2)) {
		s_bgcolor = c2;
		if (my_console == BS_CONSOLE_TTY)
			bs_bgcolor(s_bgcolor);
	}
	return 0;
}

/*  INKEY([mode])
 Get one character from the keyboard.
 mode = 0 (default): wait for input
 mode = 1: no wait for input
 mode = -1: scan only (the key is left in the buffer)  */
int
bs_builtin_inkey(void)
{
	Int n, n1;
	if (bs_get_next_int_arg(&n) == 0)
		n = 0;
	if (n > 0)
		n1 = 0;
	else if (n == 0)
		n1 = 1;
	else n1 = -1;
	n = bs_getch(n1);
	if (n == 3) {
		/*  ctrl-C  */
		bs_runtime_error("\nUser interrupt");
		return 2;  /*  Interrupt  */
	}
	bs_push_int_value(n);
	return 0;
}

/*  SCREENSIZE(idx)
 Get the width and height of the screen
 idx = 0: width, idx = 1: height
 */
int
bs_builtin_screensize(void)
{
	Int n, ival;
	bs_get_next_int_arg(&n);
	if (n == 0) {
		ival = my_width;
	} else if (n == 1) {
		ival = my_height;
	} else {
		/*  TODO: runtime exception  */
		ival = 0;
	}
	bs_push_int_value(ival);
	return 0;
}

