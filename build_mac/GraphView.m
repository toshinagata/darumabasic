//
//  GraphView.m
//  Daruma Basic
//
//  Created by Toshi Nagata on 16/01/09.
//  Copyright 2016 Toshi Nagata. All rights reserved.
//

#import "GraphView.h"
#include "screen.h"

//  CoreGraphics header
#include <ApplicationServices/ApplicationServices.h>

extern pixel_t gSyntaxColors[];

/*  Color palette  */
#if BYTES_PER_PIXEL == 2
static pixel_t sPalette[65536];
#endif

static CGContextRef s_text_context, s_graphic_context, s_active_context, s_composed_context;
static pixel_t *s_text_pixels, *s_graphic_pixels, *s_composed_pixels;

static int16_t s_redraw_x1, s_redraw_y1, s_redraw_x2, s_redraw_y2;

#define CLEAR_REDRAW ((s_redraw_x1 = s_redraw_y1 = 10000), (s_redraw_x2 = s_redraw_y2 = -1))

#if 0
static int s_path_drawflag = 1;  /*  Stroke only  */
static pixel_t s_stroke_color = (BYTES_PER_PIXEL == 2 ? 0xffff : 0xffffffff);
static pixel_t s_fill_color = 0;
#endif

GraphView *s_graphView;

static void
s_show_uptime(int n)
{
	static u_int64_t old_uptime;
	u_int64_t uptime = bs_uptime(0);
	debug_printf("%d %.3f %.3f\n", n, uptime/1000.0, (uptime - old_uptime)/1000.0);
	/*	if (uptime - old_uptime < 20000) 
	 debug_printf("<<< short interval >>>\n"); */
	old_uptime = uptime;
}

@implementation GraphView

#define CREATE_FONT 0

#if CREATE_FONT
- (void)createFontData
{
	//  The following code snippet was used in generating the bitmap data
	static u_int32_t *sBitmapBuffer;
	NSDictionary *attr;
	NSGraphicsContext *sTextContext;
	NSBitmapImageRep *rep;
	unichar ch[10];
	int i, j, k, count, xx, yy;
	u_int16_t b;
	u_int32_t uc, eu, idx;
	FILE *fp1, *fp2, *fp3;
	NSString *s;
	char buf[64], *p;
	char buf2[16][80];
	
	/*  See graph.h for details  */
	static u_int16_t sConvTable[65536];
	static u_int8_t sFontData[16*188];
	static u_int8_t sKanjiData[32*11844];
	
	sBitmapBuffer = (u_int32_t *)calloc(sizeof(u_int32_t), 1024);
	rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:(u_int8_t **)&sBitmapBuffer pixelsWide:32 pixelsHigh:32 bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
	sTextContext = [NSGraphicsContext graphicsContextWithBitmapImageRep:rep];
	[sTextContext setShouldAntialias:NO];
	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext setCurrentContext:sTextContext];
	attr = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont fontWithName:@"Sazanami-Gothic-Regular" size:15], NSFontAttributeName, [NSColor whiteColor], NSForegroundColorAttributeName, nil];
	
	fp2 = fopen("fontdata.ascii", "w");
	fp3 = fopen("../../euc-jis-2004-with-char-u8.txt", "r");
	count = 0;
	for (i = 0; i < 65536; i++)
		sConvTable[i] = 0xffff;
	while (fgets(buf, sizeof buf, fp3) != NULL) {
		j = k = 0;
		for (i = 0; buf[i] != 0; i++) {
			if (buf[i] == '\t') {
				if (j == 0)
					j = i + 3;
				else if (k == 0) {
					k = i + 3;
					break;
				}
			}
		}
		eu = strtol(buf + j, NULL, 16);
		uc = strtol(buf + k, &p, 16);
		if (*p == '+')
			uc = uc * 65536 + strtol(p + 1, &p, 16);  /*  Two-character pair  */
		if (uc >= 65536) {
			count++;
			debug_printf("%d: Cannot convert: %s", count, buf);
			continue;
		}
		if (eu >= 0x8fa1a1) {
			if (eu >= 0x8feea1) {
				idx = 10434 + (((eu >> 8) & 0xff) - 0xee) * 94 + ((eu & 0xff) - 0xa1);
			} else {
				idx = 9024 + (((eu >> 8) & 0xff) - 0xa1) * 94 + ((eu & 0xff) - 0xa1);
			}
		} else if (eu >= 0xa1a1) {
			idx = 188 + (((eu >> 8) & 0xff) - 0xa1) * 94 + ((eu & 0xff) - 0xa1);
		} else if (eu >= 0x8ea1) {
			idx = 94 + ((eu & 0xff) - 0xa1);
		} else if (eu >= 0x21 && eu <= 0x7e) {
			idx = eu - 0x21;
		} else continue; /* Silently skip */
		sConvTable[uc] = idx;
	}
	fclose(fp3);
	memset(sFontData, 0, sizeof(sFontData));
	memset(sKanjiData, 0, sizeof(sKanjiData));
	for (uc = 0; uc < 65536; uc++) {
		idx = sConvTable[uc];
		if (idx == 0xffff)
			continue;
		ch[0] = uc;
		s = [[NSString alloc] initWithCharacters:ch length:1];
		memset(sBitmapBuffer, 0, sizeof(sBitmapBuffer[0]) * 1024);
		[s drawWithRect:NSMakeRect(0, 0, 16, 32) options:NSStringDrawingUsesLineFragmentOrigin attributes:attr];
		[s release];
		for (yy = 0; yy < 16; yy++) {
			b = 0;
			if (idx < 188) {
				for (xx = 0; xx < 8; xx++) {
					if (sBitmapBuffer[(yy + 6) * 32 + xx] != 0)
						b |= (1 << xx);
				}
				sFontData[idx * 16 + yy] = b;
			} else {
				for (xx = 0; xx < 16; xx++) {
					if (sBitmapBuffer[(yy + 6) * 32 + xx] != 0)
						b |= (1 << xx);
				}
				j = (idx - 188) * 32 + yy * 2;
				sKanjiData[j] = b & 0xff;
				sKanjiData[j + 1] = (b >> 8) & 0xff;
			}
		}
	}
	for (idx = 0; idx < 12032; idx++) {
		if (idx < 188) {
			i = idx % 8;
			if (i == 0) {
				for (j = 0; j < 8; j++) {
					k = j + idx;
					if (k < 94)
						eu = k + 0x21;
					else
						eu = k + 0x8ea1;
					fprintf(fp2, "%04X%s", eu, (j == 7 ? "\n" : "     "));
				}
				memset(buf2, ' ', sizeof(buf2));
			}
			for (yy = 0; yy < 16; yy++) {
				b = sFontData[idx * 16 + yy];
				for (xx = 0; xx < 8; xx++) {
					buf2[yy][i * 9 + xx] = ((b & (1 << xx)) != 0 ? '*' : ' ');
				}
			}
			if (i == 7 || idx == 187) {
				for (yy = 0; yy < 16; yy++) {
					buf2[yy][70] = '\n';
					buf2[yy][71] = 0;
					fputs(buf2[yy], fp2);
				}
			}
		} else {
			i = (idx - 188) % 4;
			if (i == 0) {
				for (j = 0; j < 4; j++) {
					k = j + idx;
					if (k < 9024) {
						eu = 0xa1a1 + ((k - 188) / 94) * 256 + (k - 188) % 94;
						fprintf(fp2, "%04X%s", eu, (j == 3 ? "\n" : "             "));
					} else {
						if (idx < 10434)
							eu = 0x8fa1a1 + ((k - 9024) / 94) * 256 + (k - 9024) % 94;
						else
							eu = 0x8feea1 + ((k - 10434) / 94) * 256 + (k - 10434) % 94;
						fprintf(fp2, "%06X%s", eu, (j == 3 ? "\n" : "           "));
					}
				}
				memset(buf2, ' ', sizeof(buf2));
			}
			for (yy = 0; yy < 16; yy++) {
				b = sKanjiData[(idx - 188) * 32 + yy * 2] + (sKanjiData[(idx - 188) * 32 + yy * 2 + 1] * 256);
				for (xx = 0; xx < 16; xx++) {
					buf2[yy][i * 17 + xx] = ((b & (1 << xx)) != 0 ? '*' : ' ');
				}
			}
			if (i == 3 || idx == 12031) {
				for (yy = 0; yy < 16; yy++) {
					buf2[yy][70] = '\n';
					buf2[yy][71] = 0;
					fputs(buf2[yy], fp2);
				}
			}
		}
	}
	[NSGraphicsContext restoreGraphicsState];
	fclose(fp2);
	
	/*  Write the font data to file  */
	fp2 = fopen("fontdata.c", "w");
	fprintf(fp2, "unsigned short gConvTable[%d] = {\n  ", (int)sizeof(sConvTable)/2);
	for (uc = 0; uc < 65536; uc++) {
		fprintf(fp2, "%d", sConvTable[uc]);
		if (uc == 65535)
			fputs("\n};\n", fp2);
		else if (uc % 16 == 15)
			fputs(",\n  ", fp2);
		else fputc(',', fp2);		
	}
	fprintf(fp2, "unsigned char gFontData[%d] = {\n  ", (int)sizeof(sFontData));
	for (i = 0; i < sizeof(sFontData); i++) {
		fprintf(fp2, "%d", sFontData[i]);
		if (i == sizeof(sFontData) - 1)
			fputs("\n};\n", fp2);
		else if (i % 16 == 15)
			fputs(",\n  ", fp2);
		else fputc(',', fp2);
	}
	fprintf(fp2, "unsigned char gKanjiData[%d] = {\n  ", (int)sizeof(sKanjiData));
	for (i = 0; i < sizeof(sKanjiData); i++) {
		fprintf(fp2, "%d", sKanjiData[i]);
		if (i == sizeof(sKanjiData) - 1)
			fputs("\n};\n", fp2);
		else if (i % 32 == 31)
			fputs(",\n  ", fp2);
		else fputc(',', fp2);
	}
	fclose(fp2);
	fp1 = fopen("fontdata.bin", "wb");
	
	i = NSSwapHostIntToLittle(sizeof(sConvTable) / 2);
	fwrite(&i, 1, 4, fp1);
	i = NSSwapHostIntToLittle(sizeof(sFontData));
	fwrite(&i, 1, 4, fp1);
	i = NSSwapHostIntToLittle(sizeof(sKanjiData));
	fwrite(&i, 1, 4, fp1);
	
	for (uc = 0; uc < 65536; uc++) {
		/*  Convert to little endian  */
		u_int16_t si;
		((char *)(&si))[0] = sConvTable[uc] & 0xff;
		((char *)(&si))[1] = (sConvTable[uc] >> 8) & 0xff;
	}
	fwrite(sConvTable, 1, sizeof(sConvTable), fp1);
	fwrite(sFontData, 1, sizeof(sFontData), fp1);
	fwrite(sKanjiData, 1, sizeof(sKanjiData), fp1);
	fclose(fp1);
	
}
#endif

- (void)awakeFromNib
{
	NSRect frame = [self frame];
	
	my_width = frame.size.width - 8;
	my_height = frame.size.height - 8;
	
	my_fb_width = my_width;
	my_fb_height = my_height;
	
	s_graphView = self;
	
	[self setBounds:NSMakeRect(-4, -4, my_width + 8, my_height + 8)];
	[self setNeedsDisplay:YES];
	
#if CREATE_FONT
	[self createFontData];
#endif

}

- (void)drawRect:(NSRect)dirtyRect
{
	bs_draw_platform(&dirtyRect);
}

//  Key inputs
#define kKeyBufferMax 128
static u_int16_t sKeyBuffer[kKeyBufferMax];
static int sKeyBufferBase, sKeyBufferCount;

- (BOOL)acceptsFirstResponder
{
	return YES;
}

static void
s_RegisterKeyDown(u_int16_t *ch)
{
	while (*ch != 0) {
		if (sKeyBufferCount < kKeyBufferMax) {
			sKeyBuffer[(sKeyBufferBase + sKeyBufferCount) % kKeyBufferMax] = *ch;
			sKeyBufferCount++;
		}
		ch++;
	}
}

/*  If true, then draw the screen with white background  */
/*  (For creating manuals: black background is not friendly to printers  */
static unsigned char s_reverse = 0;

static void
s_ScreenShot(void)
{
	NSImage *image;
	NSBitmapImageRep *bitmapImageRep;
	NSData *tiffData, *pngData;
	NSString *path;
	NSAffineTransform *tr;
	static int s_shotnum = 0;
	NSRect rect = NSMakeRect(0, 0, my_width + 8, my_height + 8);
	if ([[NSApp currentEvent] modifierFlags] & NSShiftKeyMask)
		s_reverse = 1;
	image = [[NSImage alloc] initWithSize:rect.size];
	[image lockFocus];
	[[NSColor blackColor] set];
	NSRectFill(rect);
	tr = [NSAffineTransform transform];
	[tr translateXBy:4 yBy:4];
	[tr set];
	bs_redraw(0, 0, my_width, my_height);
	bs_draw_platform(&rect);
	s_reverse = 0;
	[image unlockFocus];
	tiffData = [image TIFFRepresentationUsingCompression:NSTIFFCompressionNone factor:1.0];
	bitmapImageRep = [NSBitmapImageRep imageRepWithData:tiffData];
	pngData = [bitmapImageRep representationUsingType:NSPNGFileType properties:nil];
	path = [[NSString stringWithFormat:@"%s/../screenshot%03d.png", bs_basedir, ++s_shotnum]
			stringByStandardizingPath];
	[pngData writeToFile:path atomically:YES];
	[image release];
}

/*  TODO: is it possible to interface with anthy for Japanese input?  */
/*  (https://osdn.jp/projects/anthy/)  */

- (void)keyDown: (NSEvent *)theEvent
{
	unichar ch[6];
	ch[0] = [[theEvent characters] characterAtIndex:0];
	ch[1] = 0;
	switch (ch[0]) {
		case NSUpArrowFunctionKey: ch[0] = 0x1b; ch[1] = '['; ch[2] = 'A'; ch[3] = 0; break;
		case NSDownArrowFunctionKey: ch[0] = 0x1b; ch[1] = '['; ch[2] = 'B'; ch[3] = 0; break;
		case NSLeftArrowFunctionKey: ch[0] = 0x1b; ch[1] = '['; ch[2] = 'D'; ch[3] = 0; break;
		case NSRightArrowFunctionKey: ch[0] = 0x1b; ch[1] = '['; ch[2] = 'C'; ch[3] = 0; break;
		case NSDeleteFunctionKey: ch[0] = 127; break;
		case NSBreakFunctionKey: ch[0] = 27; break;
		case NSF3FunctionKey: if ([theEvent modifierFlags] & NSAlternateKeyMask) s_ScreenShot(); ch[0] = 0; break;
		default: break;
	}
	s_RegisterKeyDown(ch);
}

@end

#if 0
#pragma mark ====== External API ======
#endif

int
bs_init_screen_platform(void)
{
	//  my_fb_width and my_fb_height should be set before calling this
	CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
	s_text_pixels = (pixel_t *)calloc(sizeof(pixel_t), my_fb_width * my_fb_height);
	s_graphic_pixels = (pixel_t *)calloc(sizeof(pixel_t), my_fb_width * my_fb_height);
	s_composed_pixels = (pixel_t *)calloc(sizeof(pixel_t), my_fb_width * my_fb_height);
	s_text_context = CGBitmapContextCreate(s_text_pixels,
										   my_fb_width, my_fb_height,
										   8, 4 * my_fb_width, space,
										   kCGImageAlphaPremultipliedLast);
	s_graphic_context = CGBitmapContextCreate(s_graphic_pixels,
											  my_fb_width, my_fb_height,
											  8, 4 * my_fb_width, space,
											  kCGImageAlphaPremultipliedLast);
	s_composed_context = CGBitmapContextCreate(s_composed_pixels,
											  my_fb_width, my_fb_height,
											  8, 4 * my_fb_width, space,
											  kCGImageAlphaPremultipliedLast);
	CGColorSpaceRelease(space);

	CLEAR_REDRAW;

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

	/*	NSRect aRect = NSMakeRect(x, y, width, height);
	bs_lock();
	[s_graphView setNeedsDisplayInRect:aRect];
	bs_unlock(); */
}

void
bs_update_screen(void)
{
	if (!my_suppress_update) {
		if (s_redraw_x1 < s_redraw_x2 && s_redraw_y1 < s_redraw_y2) {
			NSRect aRect = NSMakeRect(s_redraw_x1, s_redraw_y1, s_redraw_x2 - s_redraw_x1, s_redraw_y2 - s_redraw_y1);
			[s_graphView setNeedsDisplayInRect:aRect];
			[s_graphView displayIfNeeded];
			CLEAR_REDRAW;
		}
	}
}

void
bs_draw_platform(void *ref)
{
	CGContextRef cref = [[NSGraphicsContext currentContext] graphicsPort];
	CGRect r, rr;
	CGColorRef color;
	CGImageRef image;
	NSRect dirtyRect = *((NSRect *)ref);
	int x, y, x1, y1, x2, y2, mx1, mx2, my1, my2, cx1, cx2, cy1, cy2, width, width2;
	pixel_t tpix;
	int32_t ofs, ofs2;
	
	if (s_reverse)
		color = CGColorCreateGenericRGB(1, 1, 1, 1);
	else
		color = CGColorCreateGenericRGB(0, 0, 0, 1);
	CGContextSetFillColorWithColor(cref, color);
	r = CGRectMake(dirtyRect.origin.x, dirtyRect.origin.y, dirtyRect.size.width, dirtyRect.size.height);
	CGContextFillRect(cref, r);
	CGColorRelease(color);
	if (s_text_context == (CGContextRef)0)
		return;
	CGContextSaveGState(cref);
	rr = CGRectMake(0, my_height - my_fb_height, my_fb_width, my_fb_height);

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
	
	ofs = (my_fb_height - my_height + y1) * my_fb_width + x1;
	ofs2 = y1 * my_fb_width + x1;
	width = x2 - x1;
	width2 = width;		
	for (y = y1; y < y2; y++) {
		for (x = x1; x < x2; x++) {
			tpix = s_text_pixels[ofs];
			if (tpix == 0 && x < cx2 && x >= cx1 && y < cy2 && y >= cy1) {
				if (s_reverse == 0)
					tpix = RGBFLOAT(0.5, 0.5, 0.5);  /*  cursor color: 50% gray  */
			}
			if (tpix == 0)
				tpix = s_graphic_pixels[ofs];
			if (s_reverse) {
				int r, g, b;
				if (tpix == gSyntaxColors[9])
					tpix = RGBFLOAT(0.2, 0.2, 0.2);
				else if (tpix == gSyntaxColors[8])
					tpix = RGBFLOAT(0.7, 0.5, 0.8);
				r = REDCOMPINTMAX - REDCOMPINT(tpix);
				g = GREENCOMPINTMAX - GREENCOMPINT(tpix);
				b = BLUECOMPINTMAX - BLUECOMPINT(tpix);
				tpix = RGBAINT(r, g, b, ALPHACOMPINTMAX);
			}
			s_composed_pixels[ofs2] = tpix;
			ofs++;
			ofs2++;
		}
		ofs += my_fb_width - width;
		ofs2 += my_fb_width - width2;
	}
		
	/*  Only draw inside the 'visible' screen  */
	CGContextClipToRect(cref, CGRectMake(0, 0, my_width, my_height));

	/*  Draw graphic layer  */
	image = CGBitmapContextCreateImage(s_composed_context);
	CGContextDrawImage(cref, rr, image);
	CGImageRelease(image);
	
#if 0
	/*  Draw text layer  */
	CGContextBeginTransparencyLayer(cref, NULL);
	image = CGBitmapContextCreateImage(s_text_context);
	CGContextDrawImage(cref, r, image);
	CGImageRelease(image);
	if (my_show_cursor) {
		r.origin.x = (my_cursor_x + my_cursor_xofs) * 8;
		r.origin.y = my_height - (my_cursor_y + my_cursor_yofs + 1) * 16;
		r.size.width = 8;
		r.size.height = 16;
		color = CGColorCreateGenericRGB(1, 1, 1, 0.6);
		CGContextSetFillColorWithColor(cref, color);
		CGContextFillRect(cref, r);
		CGColorRelease(color);
	}
	CGContextEndTransparencyLayer(cref);
#endif
	
	CGContextRestoreGState(cref);
}

/*  Get the key input.   */
int
bs_getch_with_timeout(int32_t usec)
{
	int ch;
	NSEvent *event;
	
	if (usec < 0)
		usec = 0;

	/*  Update display (if not running)  */
	if (usec > 0 || gRunMode == BS_RUNMODE_NONE)
		bs_update_screen();
	
	/*  Discard the events already in the queue  */
	while ((event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES]) != NULL) {
		debug_printf("event type:%d\n", (int)[event type]);
		[NSApp sendEvent:event];
	}
	
	/*  Dispatch event and wait for key input  */
	if (sKeyBufferCount == 0 && usec > 0) {
		NSDate *date = [NSDate dateWithTimeIntervalSinceNow:usec / 1000000.0];
		NSEvent *event = [NSApp nextEventMatchingMask:NSKeyDownMask untilDate:date inMode:NSDefaultRunLoopMode dequeue:YES];
		if (event != nil) {
			[NSApp sendEvent:event];
		}
	}
	if (sKeyBufferCount > 0) {
		ch = sKeyBuffer[sKeyBufferBase];
		sKeyBufferBase = (sKeyBufferBase + 1) % kKeyBufferMax;
		sKeyBufferCount--;
	} else ch = -1;
	
	return ch;
}

int
bs_select_active_buffer(int active)
{
	if (active == TEXT_ACTIVE) {
		s_active_context = s_text_context;
	} else if (active == GRAPHIC_ACTIVE) {
		s_active_context = s_graphic_context;
	} else {
		/*  None  */
	}
	return 0;
}

int
tty_puts(const char *s)
{
	fputs(s, stderr);
	return 0;
}

int
bs_gmode_platform(int gmode)
{
	if (gmode == 1) {
		float f;
		my_width = 320;
		my_height = 240;
		f = (my_width * 4.0 / my_fb_width);
		[s_graphView setBounds:NSMakeRect(-f, -f, my_width + 2 * f, my_height + 2 * f)];
	} else {
		my_width = my_fb_width;
		my_height = my_fb_height;
		[s_graphView setBounds:NSMakeRect(-4, -4, my_width + 8, my_height + 8)];
	}
	[s_graphView setNeedsDisplay:YES];
	return 0;
}

const char *
bs_platform_name(void)
{
	return "mac";
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
/*	
	CGRect r = {x, y, width, height};
	CGContextClearRect(s_active_context, r);
	bs_redraw(x, y, width, height);
*/
}

void
bs_put_pattern(const pixel_t *p, int x, int y, int width, int height)
{
	int i, j, dx, dy, x1, x2, y1, y2, d1, d2;
	pixel_t *pd;
	x1 = x;
	y1 = y;
	dx = dy = 0;
	x2 = x + width;
	y2 = y + height;
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
	p += dy * width + dx;
	pd = (s_active_context == s_text_context ? s_text_pixels : s_graphic_pixels);
	pd += my_fb_width * (my_fb_height - y2) + x1;
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
	dx = dy = 0;
	x2 = x + width;
	y2 = y + height;
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
	pd = (s_active_context == s_text_context ? s_text_pixels : s_graphic_pixels);
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
	pixel_t *basep = (s_active_context == s_text_context ? s_text_pixels : s_graphic_pixels);
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
	pixel_t *pd = (s_active_context == s_text_context ? s_text_pixels : s_graphic_pixels);
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
	p = (s_active_context == s_text_context ? s_text_pixels : s_graphic_pixels);
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

