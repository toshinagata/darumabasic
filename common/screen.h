/*
 *  screen.h
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 16/01/09.
 *  Copyright 2016 Toshi Nagata. All rights reserved.
 *
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "daruma.h"

#if __RASPBERRY_PI__
#if __BAREMETAL__
#include <fb.h>     /*  BYTES_PER_PIXEL is defined in fb.h  */
#define PIXEL_T_DEFINED 1
#else
#define BYTES_PER_PIXEL 2
#endif
#endif

#ifndef BYTES_PER_PIXEL
#define BYTES_PER_PIXEL 4
#endif

#if BYTES_PER_PIXEL == 2
#if !PIXEL_T_DEFINED
typedef u_int16_t  pixel_t;
#endif
#define RGBAINT(ri, gi, bi, ai) \
((((pixel_t)(ri)) << 11) | \
(((pixel_t)(gi)) << 5) | \
((pixel_t)(bi)))
#define RGBAFLOAT(rf, gf, bf, af) RGBAINT((rf)*31+0.5, (gf)*63+0.5, (bf)*31+0.5, 0)
#define REDCOMPINT(pix) (((pix) >> 11) & 31)
#define GREENCOMPINT(pix) (((pix) >> 5) & 63)
#define BLUECOMPINT(pix) ((pix) & 31)
#define ALPHACOMPINT(pix) (1)
#define REDCOMPINTMAX 32
#define GREENCOMPINTMAX 64
#define BLUECOMPINTMAX 32
#define ALPHACOMPINTMAX 1
#define REDCOMPONENT(pix) (REDCOMPINT(pix) / 31.0)
#define GREENCOMPONENT(pix) (GREENCOMPINT(pix) / 63.0)
#define BLUECOMPONENT(pix) (BLUECOMPINT(pix) / 31.0)
#define ALPHACOMPONENT(pix) (1.0)
#else
#if !PIXEL_T_DEFINED
typedef u_int32_t   pixel_t;
#endif
#if __RASPBERRY_PI__
#define RGBAINT(ri, gi, bi, ai) \
((((pixel_t)(ai)) << 24) | \
(((pixel_t)(ri)) << 16) | \
(((pixel_t)(gi)) << 8) | \
((pixel_t)(bi)))
#define RGBAFLOAT(rf, gf, bf, af) RGBAINT((rf)*255+0.5, (gf)*255+0.5, (bf)*255+0.5, (af)*255+0.5)
#define REDCOMPINT(pix) (((pix) >> 16) & 255)
#define GREENCOMPINT(pix) (((pix) >> 8) & 255)
#define BLUECOMPINT(pix) (((pix)) & 255)
#define ALPHACOMPINT(pix) (((pix) >> 24) & 255)
#define REDCOMPINTMAX 256
#define GREENCOMPINTMAX 256
#define BLUECOMPINTMAX 256
#define ALPHACOMPINTMAX 256
#define REDCOMPONENT(pix) (REDCOMPINT(pix) / 255.0)
#define GREENCOMPONENT(pix) (GREENCOMPINT(pix) / 255.0)
#define BLUECOMPONENT(pix) (BLUECOMPINT(pix) / 255.0)
#define ALPHACOMPONENT(pix) (ALPHACOMPINT(pix) / 255.0)
#else
#define RGBAINT(ri, gi, bi, ai) \
((((pixel_t)(ai)) << 24) | \
(((pixel_t)(bi)) << 16) | \
(((pixel_t)(gi)) << 8) | \
((pixel_t)(ri)))
#define RGBAFLOAT(rf, gf, bf, af) RGBAINT((rf)*255+0.5, (gf)*255+0.5, (bf)*255+0.5, (af)*255+0.5)
#define REDCOMPINT(pix) ((pix) & 255)
#define GREENCOMPINT(pix) (((pix) >> 8) & 255)
#define BLUECOMPINT(pix) (((pix) >> 16) & 255)
#define ALPHACOMPINT(pix) (((pix) >> 24) & 255)
#define REDCOMPINTMAX 256
#define GREENCOMPINTMAX 256
#define BLUECOMPINTMAX 256
#define ALPHACOMPINTMAX 256
#define REDCOMPONENT(pix) (REDCOMPINT(pix) / 255.0)
#define GREENCOMPONENT(pix) (GREENCOMPINT(pix) / 255.0)
#define BLUECOMPONENT(pix) (BLUECOMPINT(pix) / 255.0)
#define ALPHACOMPONENT(pix) (ALPHACOMPINT(pix) / 255.0)
#endif
#endif

#define RGBFLOAT(rf, gf, bf) RGBAFLOAT(rf, gf, bf, 1.0)

#ifdef __cplusplus
extern "C" {
#endif
	
/*  UTF16 to Font Index conversion table  */
/*  Font Index <-> EUC-JIS-2004  */
/*  0-93 <-> 21-7E (94) */
/*  94-187 <-> 8E/A1-FE (94, hankaku kana)  */
/*  188-9023 <-> A1-FE/A1-FE (94*94)  */
/*  9024-10433 <-> 8F/A1-AF/A1-FE (15*94)  */
/*  10434-12031 <-> 8F/EE-FE/A1-FE (17*94)  */
extern u_int16_t gConvTable[65536];

/*  Font data for hankaku characters (index 0-187)  */
extern u_int8_t gFontData[16*188];
/*  Font data for zenkaku characters (index 188-12031)  */
extern u_int8_t gKanjiData[32*11844];

/*  Lock/unlock shared memory
 (should be unnecessary if we are running in a single-thread mode)  */
extern void bs_lock(void);
extern void bs_unlock(void);

/*  Select active buffer for offscreen drawing  */
/*  TEXT_ACTIVE: text, GRAPHIC_ACTIVE: graphic  */
extern int bs_select_active_buffer(int active);

extern void bs_fadeout(int n);
	
enum {
	TEXT_ACTIVE = 0,
	GRAPHIC_ACTIVE,
	PHYSICAL_ACTIVE  /*  Used internally  */
};

/*  Console output  */
enum {
	BS_CONSOLE_GRAPH,
	BS_CONSOLE_TTY
};

/*  my_console == BS_CONSOLE_TTY: tty mode  */
extern u_int8_t my_console;

/*  Screen size  */
extern int16_t my_width, my_height;

/*  Frame buffer size (may be different from my_width, my_height)  */
extern int16_t my_fb_width, my_fb_height;

/*  Screen size in text unit  */
extern int16_t my_max_x, my_max_y;

/*  Tab base and width  */
extern int16_t my_tab_base, my_tab_width;
	
/*  Internal bitmap buffer  */
/*extern pixel_t *my_textscreen;
extern pixel_t *my_graphscreen;
extern pixel_t *my_composedscreen; */

/*  Bitmap buffer for user-defined patterns (to avoid excessive malloc() calls)  */
/*extern pixel_t *my_patbuffer;
extern u_int32_t my_patbuffer_size;
extern u_int32_t my_patbuffer_count; */

/*  Text cursor  */
extern int16_t my_cursor_x, my_cursor_y;
extern int16_t my_cursor_xofs, my_cursor_yofs;
	
/*  Show cursor?  */
extern u_int8_t my_show_cursor;

/*  Suppress automatic screen update  */
extern u_int8_t my_suppress_update;
	
/*  Exchange the destination buffer, i.e. text drawing to graphic buffer,
 and graphic drawing to text buffer  */
/* extern u_int8_t my_exchange_buffer; */

/*  Graphic mode  */
extern int16_t my_graphic_mode;

/*  Origin of the visible part of the screen (expansion mode)  */
/* extern int16_t my_origin_x, my_origin_y; */

/*  User-defined patterns  */
typedef struct PatInfo {
	u_int16_t width, height;   /*  Size  */
	Off_t ofs;      /*  Offset in gPatternBase[]  */
} PatInfo;

/*  Screen initialize  */
extern int bs_init_screen(void);

/*  Graphic buffer handler  */
extern void bs_redraw(int16_t x, int16_t y, int16_t width, int16_t height);

/*  Read font data   */
extern int bs_read_fontdata(const char *path);

/*  Show/hide cursor  */
extern void bs_show_cursor(int flag);

/*  Key input  */
extern u_int16_t bs_getch(int wait);
extern int bs_getline(char *buf, int size);

/*  OS dependent input function  */
extern int bs_getch_with_timeout(int32_t usec);

/*  Code conversion  */
extern u_int16_t bs_utf8_to_utf16(const char *s, char **outpos);
extern char *bs_utf16_to_utf8(u_int16_t uc, char *s);

extern int bs_character_width(u_int16_t uc);

extern void bs_scroll(int16_t bufindex, int16_t x, int16_t y, int16_t width, int16_t height, int16_t dx, int16_t dy);
extern void bs_erase_to_eol(void);
extern void bs_cls(void);
extern int bs_locate(int x, int y);
extern pixel_t bs_tcolor(pixel_t col);
extern pixel_t bs_bgcolor(pixel_t col);
extern void bs_puts(const char *s);
extern void bs_puts_format(const char *fmt, ...);

extern void bs_gputs(const char *s, int x, int y, pixel_t fcolor, pixel_t bcolor);

extern void bs_update_screen(void);
	
/*  Debug message  */
extern int tty_puts(const char *s);

extern volatile u_int8_t g_init_screen_done;
	
/*  Platform specific procedures  */
extern int bs_init_screen_platform(void);
extern void bs_draw_platform(void *ref);
extern int bs_gmode_platform(int gmode);

/*  Platform specific drawing functions  */
extern void bs_clear_box(int x, int y, int width, int height);
extern void bs_put_pattern(const pixel_t *p, int x, int y, int width, int height);
extern void bs_get_pattern(pixel_t *p, int x, int y, int width, int height);
extern void bs_copy_pixels(int dx, int dy, int sx, int sy, int width, int height);
extern void bs_pset_nocheck(int x, int y, pixel_t color);
extern void bs_pset(int x, int y, pixel_t color);
extern void bs_hline(int x1, int x2, int y, pixel_t col);

extern int bs_builtin_locate(void);
extern int bs_builtin_cls(void);
extern int bs_builtin_clearline(void);
extern int bs_builtin_color(void);
extern int bs_builtin_tcolor(void);
extern int bs_builtin_inkey(void);
extern int bs_builtin_screensize(void);
	
#ifdef __cplusplus
}
#endif
		
#endif /* __GRAPH_H__ */
