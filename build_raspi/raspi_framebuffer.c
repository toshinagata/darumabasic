/*
 *  raspi_framebuffer.c
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 16/02/07.
 *  Copyright 2016 Toshi Nagata. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#ifndef __CONSOLE__
#if __BAREMETAL__
#if __circle__
#include "circle_bind.h"
#else
#include <fb.h>
#endif /* __circle__ */
#else
#include <linux/fb.h>
#include <sys/mman.h>
#endif
#endif

#include "daruma.h"
#include "screen.h"

extern int tty_getch(void);

static char *s_framebuffer;

#if !__BAREMETAL__
static int s_original_bpp;
static int s_fbfd = 0;
#endif

static pixel_t *s_text_pixels, *s_graphic_pixels, *s_active_pixels;

static int16_t s_redraw_x1, s_redraw_y1, s_redraw_x2, s_redraw_y2;

#define CLEAR_REDRAW ((s_redraw_x1 = s_redraw_y1 = 10000), (s_redraw_x2 = s_redraw_y2 = -1))

static u_int8_t s_expand;

static void
s_bs_raspi_framebuffer(void)
{
#ifndef __CONSOLE__
#if __BAREMETAL__
#if __circle__
	int i;
	bs_raspibm_init_framebuffer();
	s_framebuffer = bs_raspibm_get_framebuffer();
#else
	fb_init(0, 0);
	my_width = my_fb_width = fb_width;
	my_height = my_fb_height = fb_height;
	s_framebuffer = (char *)fb_addr;
#endif /* __circle__ */
#else
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	
	/*  Open frame buffer device  */
	s_fbfd = open("/dev/fb0", O_RDWR);
	if (!s_fbfd) {
		printf("Error: cannot open framebuffer device.\n");
		return;
	}
	if (ioctl(s_fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		return;
	}
	debug_printf("Frame buffer %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );
	
	s_original_bpp = vinfo.bits_per_pixel;
	vinfo.bits_per_pixel = 16;
	if (ioctl(s_fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
		printf("Error setting variable information.\n");
		return;
	}
	
	my_width = my_fb_width = vinfo.xres;
	my_height = my_fb_height = vinfo.yres;

	if (ioctl(s_fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
		return;
	}

	/*  Map the memory buffer  */
	s_framebuffer = (char *)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, s_fbfd, 0);
	if (s_framebuffer == (char *)-1) {
		printf("Failed to mmap.\n");
		return;
	}
	debug_printf("mmap'ed buffer %p\n", mmap);
#endif
#endif
}

/*  Key input.  */
/*  The standard input should be set to the RAW mode  */
int
bs_getch_raw(void)
{
	return tty_getch();
}

#if 0
#pragma mark ====== External API ======
#endif

int
bs_init_screen_platform(void)
{
#ifndef __CONSOLE__
	s_bs_raspi_framebuffer();
	s_text_pixels = (pixel_t *)bs_calloc(sizeof(pixel_t), my_fb_width * my_fb_height);
	s_graphic_pixels = (pixel_t *)bs_calloc(sizeof(pixel_t), my_fb_width * my_fb_height);
	s_active_pixels = s_text_pixels;
	CLEAR_REDRAW;
	bs_redraw(0, 0, my_width, my_height);
#endif
	return 0;
}

void
bs_lock(void)
{
}

void
bs_unlock(void)
{
}

/*  Request redraw of a part of the screen  */
void
bs_redraw(int16_t x, int16_t y, int16_t width, int16_t height)
{
	int16_t x2, y2;
	
	x2 = x + width;
	y2 = y + height;
	if (x < s_redraw_x1)
		s_redraw_x1 = x;
	if (y < s_redraw_y1)
		s_redraw_y1 = y;
	if (x2 > s_redraw_x2)
		s_redraw_x2 = x2;
	if (y2 > s_redraw_y2)
		s_redraw_y2 = y2;
}

void
bs_update_screen(void)
{
	if (!my_suppress_update) {
		if (s_redraw_x1 < s_redraw_x2 && s_redraw_y1 < s_redraw_y2) {
			bs_draw_platform(NULL);
			CLEAR_REDRAW;
		}
	}
}

void
bs_draw_platform(void *ref)
{
	int x, y, x1, y1, x2, y2, mx1, mx2, my1, my2, cx1, cx2, cy1, cy2, width, width2;
	pixel_t tpix;
	int32_t ofs, ofs2;
	
	x1 = s_redraw_x1;
	x2 = s_redraw_x2;
	y1 = my_height - s_redraw_y2;
	y2 = my_height - s_redraw_y1;

	mx1 = my1 = 0;
	mx2 = my_width;
	my2 = my_height;

	if (x1 < mx1)
		x1 = mx1;
	if (x2 > mx2)
		x2 = mx2;
	if (y1 < my1)
		y1 = my1;
	if (y2 > my2)
		y2 = my2;

	if (my_show_cursor) {
		cx1 = my_cursor_x * 8;
		cx2 = cx1 + 8;
		cy1 = my_cursor_y * 16;
		cy2 = cy1 + 16;
	} else cx1 = cx2 = cy1 = cy2 = -1;
	
#if __BAREMETAL__
#if __circle__
	s_framebuffer = (char *)bs_raspibm_get_framebuffer();
#else
	s_framebuffer = (char *)fb_addr;  /*  May change because of double buffering  */
#endif
#endif

	if (s_expand) {
		/*  (2x2) expansion  */
		ofs = (my_fb_height - my_height + y1) * my_fb_width + x1;
		ofs2 = y1 * my_fb_width * 2 + x1 * 2;
		width = x2 - x1;
		width2 = width * 2;
		for (y = y1; y < y2; y++) {
			for (x = x1; x < x2; x++) {
				tpix = s_text_pixels[ofs];
				if (tpix == 0 && x < cx2 && x >= cx1 && y < cy2 && y >= cy1)
					tpix = RGBFLOAT(0.5, 0.5, 0.5);  /*  cursor color: 50% gray  */
				if (tpix == 0)
					tpix = s_graphic_pixels[ofs];
				((pixel_t *)s_framebuffer)[ofs2] = tpix;
				((pixel_t *)s_framebuffer)[ofs2 + 1] = tpix;
				((pixel_t *)s_framebuffer)[ofs2 + my_fb_width] = tpix;
				((pixel_t *)s_framebuffer)[ofs2 + my_fb_width + 1] = tpix;
				ofs++;
				ofs2 += 2;
			}
			ofs += my_fb_width - width;
			ofs2 += my_fb_width * 2 - width2;
		}			
	} else {
		ofs = y1 * my_fb_width + x1;
		ofs2 = y1 * my_fb_width + x1;
		width = x2 - x1;
		width2 = width;		
		for (y = y1; y < y2; y++) {
			for (x = x1; x < x2; x++) {
				tpix = s_text_pixels[ofs];
				if (tpix == 0 && x < cx2 && x >= cx1 && y < cy2 && y >= cy1)
					tpix = RGBFLOAT(0.5, 0.5, 0.5);  /*  cursor color: 50% gray  */
				if (tpix == 0)
					tpix = s_graphic_pixels[ofs];
				((pixel_t *)s_framebuffer)[ofs2] = tpix;
				ofs++;
				ofs2++;
			}
			ofs += my_fb_width - width;
			ofs2 += my_fb_width - width2;
		}
	}
}

/*  Get the key input.   */
int
bs_getch_with_timeout(int32_t usec)
{
	int n;
	u_int32_t timer = 0, wait;

	if (usec < 0)
		usec = 0;
	
	/*  Update display (if not running)  */
	if (usec > 0 || gRunMode == BS_RUNMODE_NONE)
		bs_update_screen();

	while ((n = tty_getch()) == -1) {
		if (timer >= usec)
			break;
		wait = (usec < 10000 ? usec : 10000);
		bs_usleep(wait);
		timer += wait;
	}
	return n;
}

int
bs_select_active_buffer(int active)
{
	if (active == TEXT_ACTIVE) {
		s_active_pixels = s_text_pixels;
	} else if (active == GRAPHIC_ACTIVE) {
		s_active_pixels = s_graphic_pixels;
	} else {
		/*  None  */
	}
	return 0;
}

int
bs_gmode_platform(int gmode)
{
	if (gmode == 1) {
		my_width = my_fb_width / 2;
		my_height = my_fb_height / 2;
		s_expand = 1;
	} else {
		my_width = my_fb_width;
		my_height = my_fb_height;
		s_expand = 0;
	}
	return 0;
}

const char *
bs_platform_name(void)
{
	return "raspi";
}

void
bs_fadeout(int n)
{
	int i, j, r, g, b, a;
	float rr = 0.92;
	pixel_t *p1, c;
	for (j = 0; j < 2; j++) {
		p1 = (j == 0 ? s_text_pixels : s_graphic_pixels);
		for (i = 0; i < my_fb_width * my_fb_height; i++) {
			if (n == 0)
				*p1 = 0;
			else {
				c = *p1;
				r = REDCOMPINT(c) * rr;
				g = GREENCOMPINT(c) * rr;
				b = BLUECOMPINT(c) * rr;
				a = ALPHACOMPINT(c) * rr;
				*p1 = RGBAINT(r, g, b, a);
			}
			p1++;
		}
	}
	bs_redraw(0, 0, my_fb_width, my_fb_height);
	bs_update_screen();
}

#if 0
#pragma mark ====== Drawing primitives ======
#endif

void
bs_clear_box(int x, int y, int width, int height)
{
	int x2, y2;
	if (width <= 0 || height <= 0)
		return;
	x2 = x + width - 1;
	y2 = y + height - 1;
	while (y <= y2) {
		bs_hline(x, x2, y, 0);
		y++;
	}
}

void
bs_put_pattern(const pixel_t *p, int x, int y, int width, int height)
{
	int i, j, dx, dy, x1, x2, y1, y2, d1, d2;
	pixel_t *pd;
	x1 = x;
	y1 = y;
	x2 = x + width;
	y2 = y + height;
	dx = dy = 0;
	if (x1 < 0) {
		dx = -x1;
		x1 = 0;
	}
	if (y1 < 0)
		y1 = 0;
	if (x2 >= my_width)
		x2 = my_width;
	if (y2 >= my_height) {
		dy = y2 - my_height;
		y2 = my_height;
	}
	if (x1 >= x2 || y1 >= y2)
		return;
	d1 = width - (x2 - x1);
	d2 = my_fb_width - (x2 - x1);
	pd = s_active_pixels;
	pd += my_fb_width * (my_fb_height - y2) + x1;
	p += dy * width + dx;
	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			*pd++ = *p++;
		}
		p += d1;
		pd += d2;
	}
	bs_redraw(x1, y1, x2 - x1, y2 - y1);
}

void
bs_get_pattern(pixel_t *p, int x, int y, int width, int height)
{
	int i, j, dx, dy, x1, x2, y1, y2, d1, d2;
	pixel_t *pd;
	x1 = x;
	y1 = y;
	x2 = x + width;
	y2 = y + height;
	dx = dy = 0;
	if (x1 < 0) {
		dx = -x1;
		x1 = 0;
	}
	if (y1 < 0)
		y1 = 0;
	if (x2 >= my_width)
		x2 = my_width;
	if (y2 >= my_height) {
		dy = y2 - my_height;
		y2 = my_height;
	}
	if (x1 >= x2 || y1 >= y2)
		return;
	d1 = width - (x2 - x1);
	d2 = my_fb_width - (x2 - x1);
	pd = s_active_pixels;
	pd += my_fb_width * (my_fb_height - y2) + x1;
	memset(p, 0, width * height * sizeof(pixel_t));
	p += dy * width + dx;
	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			*p++ = *pd++;
		}
		p += d1;
		pd += d2;
	}
}

void
bs_copy_pixels(int dx, int dy, int sx, int sy, int width, int height)
{
	pixel_t *basep = s_active_pixels;
	int w1, h1, yy, sy1, dy1;
	pixel_t *srcp, *dstp;
	w1 = width;
	if (sx < 0) {
		w1 += sx;
		dx -= sx;
		sx = 0;
	}
	if (dx < 0) {
		w1 += dx;
		sx -= dx;
		dx = 0;
	}
	if (sx + w1 > my_width)
		w1 = my_width - sx;
	if (dx + w1 > my_width)
		w1 = my_width - dx;
	h1 = height;
	if (sy < 0) {
		h1 += sy;
		dy -= sy;
		sy = 0;
	}
	if (dy < 0) {
		h1 += dy;
		sy -= dy;
		dy = 0;
	}
	if (sy + h1 > my_height)
		h1 = my_height - sy;
	if (dy + h1 > my_height)
		h1 = my_height - dy;
	if (sy < 0 || dy < 0)
		return;
	/*  The origin of the bitmap is left-top  */
	sy1 = my_fb_height - height - sy;
	dy1 = my_fb_height - height - dy;
	if (sy1 < dy1) {
		/*  Down scroll: move bottom to top  */
		for (yy = h1 - 1; yy >= 0; yy--) {
			srcp = basep + sx + (sy1 + yy) * my_fb_width;
			dstp = basep + dx + (dy1 + yy) * my_fb_width;
			memmove(dstp, srcp, w1 * sizeof(pixel_t));
		}
	} else {
		/*  Up scroll: move top to bottom  */
		for (yy = 0; yy < h1; yy++) {
			srcp = basep + sx + (sy1 + yy) * my_fb_width;
			dstp = basep + dx + (dy1 + yy) * my_fb_width;
			memmove(dstp, srcp, w1 * sizeof(pixel_t));
		}
	}
	bs_redraw(sx, sy, w1, h1);	
	bs_redraw(dx, dy, w1, h1);	
}

void
bs_pset_nocheck(int x, int y, pixel_t color)
{
	pixel_t *pd = s_active_pixels;
	*(pd + x + (my_fb_height - y - 1) * my_fb_width) = color;
	bs_redraw(x, y, 1, 1);
}

void
bs_pset(int x, int y, pixel_t color)
{
	if (x >= 0 && x < my_width && y >= 0 && y < my_height) {
		bs_pset_nocheck(x, y, color);
	} else {
		/*	fprintf(stderr, "pset out of range: (%d, %d)\n", x, y);  */
	}
}

void
bs_hline(int x1, int x2, int y, pixel_t col)
{
	pixel_t *p;
	int xx;
	if (x1 >= my_width || x2 < 0 || y < 0 || y >= my_height)
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= my_width)
		x2 = my_width - 1;
	p = s_active_pixels;
	p += x1 + (my_fb_height - y - 1) * my_fb_width;
	xx = x1;
	while (xx++ <= x2)
		*p++ = col;
	bs_redraw(x1, y, x2 - x1 + 1, 1);
}

#if 0
#pragma mark ====== For Debug ======
#endif

void
bs_dump_image(const char *fname, const pixel_t *p, int w, int h)
{
	int i, r, g, b, a,ofs;
	pixel_t col;
	u_int8_t buf[256];
	FILE *fp;
	fp = fopen(fname, "w");
	if (fp != NULL) {
		memset(buf, 0, 18);
		buf[2] = 2;
		buf[12] = (w & 0xff);
		buf[13] = ((w >> 8) & 0xff);
		buf[14] = (h & 0xff);
		buf[15] = ((h >> 8) & 0xff);
		buf[16] = 32;
		buf[17] = 0x28;
		fwrite(buf, 18, 1, fp);
		for (i = ofs = 0; i < w * h; i++, p++) {
			col = *p;
			r = floor(REDCOMPONENT(col) * 255 + 0.5);
			g = floor(GREENCOMPONENT(col) * 255 + 0.5);
			b = floor(BLUECOMPONENT(col) * 255 + 0.5);
			a = floor(ALPHACOMPONENT(col) * 255 + 0.5);
			buf[ofs++] = b;
			buf[ofs++] = g;
			buf[ofs++] = r;
			buf[ofs++] = a;
			if (ofs == 256 || i == w * h - 1) {
				fwrite(buf, ofs, 1, fp);
				ofs = 0;
			}
		}
		fclose(fp);
	}
}

void
bs_dump_screen(void)
{
	static int count = 0;
	int j;
	char fname[32];
	count++;
	for (j = 0; j < 2; j++) {
		pixel_t *p = (j == 0 ? s_text_pixels : s_graphic_pixels);
		snprintf(fname, sizeof fname, "%s%03d.tga", (j == 0 ? "text" : "graph"), count);
		bs_dump_image(fname, p, my_fb_width, my_fb_height);
	}
}

