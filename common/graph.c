/*
 *  graph.c
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
#include <math.h>
#include <alloca.h>

static u_int8_t s_strokeflag = 1, s_fillflag = 0;
static pixel_t s_gcolor = RGBFLOAT(1, 1, 1), s_fillcolor = 0;

static int s_last_x = 0, s_last_y = 0;

#define DIVINT(n, m) s_divint(n, m)

static inline int
s_divint(int n, int m)
{
	if (n >= 0) {
		if (m > 0)
			return (n + m / 2) / m;
		else return -((n + (-m) / 2) / (-m));
	} else {
		n = -n;
		if (m > 0)
			return -(n + m / 2) / m;
		else return (n + (-m) / 2) / (-m);
	}
}

#define s_intersect_x(x1, y1, x2, y2, clip_x, w, wx, wy) \
(x1 == x2 ? (x1 >= cx1 && x1 < cx2) : \
(w = (y2 - y1) * (clip_x - x1) / (x2 - x1) + y1, \
( (w < cy1 || w >= cy2) ? 0 : (wx = clip_x, wy = w, 1))))

#define s_intersect_y(x1, y1, x2, y2, clip_y, w, wx, wy) \
(y1 == y2 ? (y1 >= cy1 && y1 < cy2) : \
(w = (x2 - x1) * (clip_y - y1) / (y2 - y1) + x1, \
( (w < cx1 || w >= cx2) ? 0 : (wx = w, wy = clip_y, 1))))

static void
s_line(int x1, int y1, int x2, int y2)
{
	int i, wx, wy, sx, sy;
	
	/*  Clipping  */
	int cx1 = 0;
	int cy1 = 0;
	int cx2 = my_width - 1;
	int cy2 = my_height - 1;
	
	int sx1 = x1, sx2 = x2, sy1 = y1, sy2 = y2;
	
	/*  Out-of-bounds flags  */
	u_int8_t f1 = 0;
	u_int8_t f2 = 0;
	if (x1 < cx1) f1 |= 1;
	if (x1 > cx2) f1 |= 2;
	if (y1 < cy1) f1 |= 4;
	if (y1 > cy2) f1 |= 8;
	if (x2 < cx1) f2 |= 1;
	if (x2 > cx2) f2 |= 2;
	if (y2 < cy1) f2 |= 4;
	if (y2 > cy2) f2 |= 8;
	
	if ((f1 & f2) != 0)
		return;  /*  Invisible  */
	
	if (f1 != 0) {
		/*  (x1, y1) needs clipping  */
		wx = x1;
		wy = y1;
		if (!((f1 & 1) && s_intersect_x(x1, y1, x2, y2, cx1, i, wx, wy)) &&
			!((f1 & 2) && s_intersect_x(x1, y1, x2, y2, cx2, i, wx, wy)) &&
			!((f1 & 4) && s_intersect_y(x1, y1, x2, y2, cy1, i, wx, wy)) &&
			!((f1 & 8) && s_intersect_y(x1, y1, x2, y2, cy2, i, wx, wy)))
			return;  /*  Invisible  */
		x1 = wx;
		y1 = wy;
	}
	if (f2 != 0) {
		/*  (x2, y2) needs clipping  */
		wx = x2;
		wy = y2;
		if (!((f2 & 1) && s_intersect_x(x1, y1, x2, y2, cx1, i, wx, wy)) &&
			!((f2 & 2) && s_intersect_x(x1, y1, x2, y2, cx2, i, wx, wy)) &&
			!((f2 & 4) && s_intersect_y(x1, y1, x2, y2, cy1, i, wx, wy)) &&
			!((f2 & 8) && s_intersect_y(x1, y1, x2, y2, cy2, i, wx, wy)))
			return;  /*  Invisible  */
		x2 = wx;
		y2 = wy;
	}
	
	wx = (x2 > x1 ? x2 - x1 : x1 - x2);
	wy = (y2 > y1 ? y2 - y1 : y1 - y2);
	sx = (x2 > x1 ? 1 : -1);
	sy = (y2 > y1 ? 1 : -1);
	
	if (0) {
		double fabs(double);
		if (wx > wy) {
			double dr1, dr2, dy1, dy2;
			dr1 = ((double)(sy2 - sy1) / (double)(sx2 - sx1));
			dr2 = sy1 - dr1 * sx1;
			dy1 = dr1 * x1 + dr2;
			dy2 = dr1 * x2 + dr2;
			if (fabs(dy1 - y1) >= 1.0 || fabs(dy2 - y2) >= 1.0) {
				printf("clipping NG: (%d,%d)-(%d,%d)/(%d,%d)-(%d,%d)\n", sx1, sy1, sx2, sy2, x1, y1, x2, y2);
			}
		} else {
			double dr1, dr2, dx1, dx2;
			dr1 = ((double)(sx2 - sx1) / (double)(sy2 - sy1));
			dr2 = sx1 - dr1 * sy1;
			dx1 = dr1 * y1 + dr2;
			dx2 = dr1 * y2 + dr2;
			if (fabs(dx1 - x1) >= 1.0 || fabs(dx2 - x2) >= 1.0) {
				debug_printf("clipping NG: (%d,%d)-(%d,%d)/(%d,%d)-(%d,%d)\n", sx1, sy1, sx2, sy2, x1, y1, x2, y2);
			}
		}
	}
	
	if (wx > wy) {
		int e = -wx;
		for (i = 0; i <= wx; i++) {
			bs_pset_nocheck(x1, y1, s_gcolor);
			x1 += sx;
			e += 2 * wy;
			if (e >= 0) {
				y1 += sy;
				e -= 2 * wx;
			}
		}
	} else {
		int e = -wy;
		for (i = 0; i <= wy; i++) {
			bs_pset_nocheck(x1, y1, s_gcolor);
			y1 += sy;
			e += 2 * wx;
			if (e >= 0) {
				x1 += sx;
				e -= 2 * wy;
			}
		}
	}
}

#define PI 3.141592653589793

static void
s_arc(int ox, int oy, int rad, double start, double end)
{
	int x, y, f, st, wd, d, d8;
	double ct, tn;
	if (rad == 0) {
		bs_pset(ox, oy, s_gcolor);
		return;
	} else if (rad < 0)
		return;
	d = ceil(0.7071067811865476 * rad);
	if (end - start == 360.0) {
		start = 0.0;
		end = 360.0;
	}
	if (start == 0.0) {
		st = 0;
	} else {
		start -= floor(start / 360.0) * 360.0;
		start *= PI / 180.0;
		ct = -cos(start) / sin(start);
		tn = tan(start);
		if (fabs(tn) <= fabs(ct)) {
			tn += (start >= PI * 1.74 ? 8 : (start >= PI * 0.74 && start <= PI * 1.26 ? 4 : 0));
		} else {
			tn = ct + (start >= PI * 0.24 && start <= PI * 0.76 ? 2 : 6);
		}
		st = floor(tn * d);
	}
	if (end == 360.0) {
		wd = 8 * d - st;
	} else {
		end -= floor(start / 360.0) * 360.0;
		end *= PI / 180.0;
		ct = -cos(end) / sin(end);
		tn = tan(end);
		if (fabs(tn) <= fabs(ct)) {
			tn += (end >= PI * 1.74 ? 8 : (end >= PI * 0.74 && end <= PI * 1.26 ? 4 : 0));
		} else {
			tn = ct + (end >= PI * 0.24 && end <= PI * 0.76 ? 2 : 6);
		}
		wd = ceil(tn * d) - st;
	}
	d8 = d * 8;
	if (wd < 0)
		wd += d8;
	st -= d8;

	x = rad;
	y = 0;
	f = 3 - 2 * rad;
	while (x >= y) {
		if ((y - st) % d8 <= wd)
			bs_pset(ox + x, oy + y, s_gcolor);
		if ((2 * d - y - st) % d8 <= wd)
			bs_pset(ox + y, oy + x, s_gcolor);
		if ((2 * d + y - st) % d8 <= wd)
			bs_pset(ox - y, oy + x, s_gcolor);
		if ((4 * d - y - st) % d8 <= wd)
			bs_pset(ox - x, oy + y, s_gcolor);
		if ((4 * d + y - st) % d8 <= wd)
			bs_pset(ox - x, oy - y, s_gcolor);
		if ((6 * d - y - st) % d8 <= wd)
			bs_pset(ox - y, oy - x, s_gcolor);
		if ((6 * d + y - st) % d8 <= wd)
			bs_pset(ox + y, oy - x, s_gcolor);
		if ((8 * d - y - st) % d8 <= wd)
			bs_pset(ox + x, oy - y, s_gcolor);
		if (f >= 0) {
			x--;
			f -= 4 * x;
		}
		y++;
		f += 4 * y + 2;
	}
}

static void
s_obarc(int ox, int oy, int rx, int ry, double start, double end, int drawflag)
{
	int a, b, x, y, f, h, d, dx, dy, st, wd, d8, dd;
	int px, py, qx, qy, fy0, fy1, fx;
	u_int8_t wf, ff, fill, stroke;
	double ct, tn;
	
	if (rx < 0 || ry < 0)
		return;
	stroke = drawflag & 1;
	fill = drawflag & 2;
	if (rx == 0) {
		if (fill || stroke) {
			if (ry == 0)
				bs_pset(ox, oy, s_gcolor);
			else
				s_line(ox, oy - ry, ox, oy + ry);
		}
		return;
	} else if (ry == 0) {
		if (fill || stroke)
			s_line(ox - rx, oy, ox + rx, oy);
		return;
	}
	
	dx = ceil(0.7071067811865476 * rx);
	dy = ceil(0.7071067811865476 * ry);
	d = (dx > dy ? dx : dy);
	
	if (end - start == 360.0) {
		start = 0.0;
		end = 360.0;
	}
	if (start == 0.0) {
		st = 0;
	} else {
		start -= floor(start / 360.0) * 360.0;
		start *= PI / 180.0;
		ct = 1.41421356 * cos(start);
		tn = 1.41421356 * sin(start);
		if (fabs(tn) <= fabs(ct)) {
			if (start >= PI * 1.74)
				tn += 8;
			else if (start >= PI * 0.74 && start <= PI * 1.26)
				tn = 4 - tn;
		} else {
			if (start >= PI * 0.24 && start <= PI * 0.76)
				tn = 2 - ct;
			else tn = 6 + ct;
		}
		st = floor(tn * d + 0.5);
	}
	if (end == 360.0) {
		wd = 8 * d - st;
		end = 0.0;
	} else {
		end -= floor(start / 360.0) * 360.0;
		end *= PI / 180.0;
		ct = 1.41421356 * cos(end);
		tn = 1.41421356 * sin(end);
		if (fabs(tn) <= fabs(ct)) {
			if (end >= PI * 1.74)
				tn += 8;
			else if (end >= PI * 0.74 && end <= PI * 1.26)
				tn = 4 - tn;
		} else {
			if (end >= PI * 0.24 && end <= PI * 0.76)
				tn = 2 - ct;
			else tn = 6 + ct;
		}
		wd = floor(tn * d + 0.5) - st;
	}
	d8 = d * 8;
	if (wd < 0)
		wd += d8;
	st -= d8;
	qx = floor(rx * cos(end) + 0.5);
	qy = floor(ry * sin(end) + 0.5);
	px = floor(rx * cos(start) + 0.5) - qx;
	py = floor(ry * sin(start) + 0.5) - qy;
	a = ry * ry;
	b = rx * rx;
	x = rx;
	y = 0;
	f = -2 * rx * a + a + 2 * b;
	h = -4 * rx * a + 2 * a + b;
	fy0 = fy1 = -1;
	if (py < 0) {
		px = -px;
		py = -py;
	}
	while (x >= 0) {
		wf = 0;
		if (x >= dx || y < dy) {
			dd = (y * d + dy / 2) / dy;
			if ((dd - st) % d8 <= wd)
				wf |= 1;
			if ((4 * d - dd - st) % d8 <= wd)
				wf |= 2;
			if ((4 * d + dd - st) % d8 <= wd)
				wf |= 4;
			if ((8 * d - dd - st) % d8 <= wd)
				wf |= 8;
		} else {
			dd = (x * d + dx / 2) / dx;
			if ((2 * d - dd - st) % d8 <= wd)
				wf |= 1;
			if ((2 * d + dd - st) % d8 <= wd)
				wf |= 2;
			if ((6 * d - dd - st) % d8 <= wd)
				wf |= 4;
			if ((6 * d + dd - st) % d8 <= wd)
				wf |= 8;
		}
		ff = 0;
		if (wf & 1) {
			if (fill && fy0 != y) {
				ff = 1;
				if (py == 0 || wf & 2)
					fx = -x;
				else {
					fx = qx + DIVINT(px * (y - qy), py);
					if (wf & 2)
						fx = -x;
					if (fx < -x)
						fx = -x;
					else if (fx > x)
						fx = x;
				}
				bs_hline(ox + fx, ox + x, oy + y, s_fillcolor);
			}
			if (stroke)
				bs_pset(ox + x, oy + y, s_gcolor);
		}
		if (wf & 2) {
			if (fill && fy0 != y && ff == 0) {
				ff = 1;
				if (py == 0)
					fx = x;
				else {
					fx = qx + DIVINT(px * (y - qy), py);
					if (fx > x)
						fx = x;
					else if (fx < -x)
						fx = -x;
				}
				bs_hline(ox - x, ox + fx, oy + y, s_fillcolor);
			}
			if (stroke)
				bs_pset(ox - x, oy + y, s_gcolor);
		}
		if (ff)
			fy0 = y;
		ff = 0;
		if (wf & 8) {
			if (fill && fy1 != y) {
				ff = 1;
				if (py == 0 || wf & 4)
					fx = -x;
				else {
					fx = qx + DIVINT(px * (-y - qy), py);
					if (fx < -x)
						fx = -x;
					else if (fx > x)
						fx = x;
				}
				bs_hline(ox + fx, ox + x, oy - y, s_fillcolor);
			}
			if (stroke)
				bs_pset(ox + x, oy - y, s_gcolor);
		}
		if (wf & 4) {
			if (fill && fy1 != y && ff == 0) {
				ff = 1;
				if (py != 0) {
					fx = qx + DIVINT(px * (-y - qy), py);
					if (fx > x)
						fx = x;
					else if (fx < -x)
						fx = -x;
				} else fx = x;
				bs_hline(ox - x, ox + fx, oy - y, s_fillcolor);
			}
			if (stroke)
				bs_pset(ox - x, oy - y, s_gcolor);
		}
		if (ff)
			fy1 = y;
		if (f >= 0) {
			x--;
			f -= 4 * a * x;
			h -= a * (4 * x - 2);
		}
		if (h < 0) {
			y++;
			f += b * (4 * y + 2);
			h += 4 * b * y;
		}
	}
}

static void
s_polyfill(Int *pts, int npts)
{
	int x1, y1, x2, y2, y3, dy1, dy2, dd, i, j, n;
	int16_t *nodes;
	u_int8_t *nodeflags;
	nodes = alloca((sizeof(int16_t) * 2 + sizeof(u_int8_t)) * npts);
	if (nodes == NULL) {
		debug_printf("Cannot allocate memory for drawing polylines\n");
		return;
	}
	nodeflags = (u_int8_t *)(nodes + npts * 2);
	memset(nodeflags, 0, npts);
	y2 = pts[npts * 2 - 1];
	y1 = pts[1];
	dy1 = y1 - y2;
	for (i = 0; i <= npts; i++) {
		/*  The first point is processed twice. Otherwise, the algorithm may
		    fail when initial dy1 == 0.  */
		if (i < npts - 1)
			y2 = pts[i * 2 + 3];
		else y2 = pts[(i + 1 - npts) * 2 + 1];
		dy2 = y2 - y1;
		if (dy2 == 0) {
			nodeflags[i] = 2;  /*  This edge is horizontal  */
		} else {
			if ((dy1 > 0 && dy2 > 0) || (dy1 < 0 && dy2 < 0))
				nodeflags[i % npts] = 1;  /*  This edge is not summit nor valley  */
			dy1 = dy2;  /*  dy1 is the 'last slope direction'  */
		}
		y1 = y2;
	}
	y1 = -10000000;
	y2 = 10000000;
	for (i = 0; i < npts; i++) {
		y3 = pts[i * 2 + 1];
		if (y1 < y3)
			y1 = y3;
		if (y2 > y3)
			y2 = y3;
	}
	for (y3 = y2; y3 <= y1; y3++) {
		if (y3 < 0 || y3 >= my_height)
			continue;
		n = 0;
		for (i = 0; i < npts; i++) {
			x1 = pts[i * 2];
			dy1 = pts[i * 2 + 1];
			if (i == npts - 1) {
				x2 = pts[0];
				dy2 = pts[1];
			} else {
				x2 = pts[i * 2 + 2];
				dy2 = pts[i * 2 + 3];
			}
			dy1 -= y3;
			dy2 -= y3;
			dd = dy2 - dy1;
			if ((dy1 > 0 && dy2 > 0) || (dy1 < 0 && dy2 < 0))
				continue;
			if (dy1 == 0 && nodeflags[i])
				continue;
			if ((dy1 > 0 && dy1 + dy2 >= 0) || (dy1 < 0 && dy1 + dy2 <= 0)) {
				x1 = x1 - DIVINT(dy1 * (x2 - x1), dd);
			} else {
				x1 = x2 - DIVINT(dy2 * (x2 - x1), dd);
			}
			for (j = 0; j < n; j++) {
				if (nodes[j] >= x1)
					break;
			}
			if (j < n)
				memmove(nodes + j + 1, nodes + j, sizeof(nodes[0]) * (n - j));
			nodes[j] = x1;
			n++;
		}
		if (n % 2 != 0) {
			debug_printf("number of nodes is odd\n");
			continue;
		}
		for (i = 0; i < n; i += 2)
			bs_hline(nodes[i], nodes[i + 1], y3, s_fillcolor);
	}
}

static void
s_poly(Int *pts, int npts, int drawflag)
{
	int i;
	if (drawflag & 2)
		s_polyfill(pts, npts);
	if (drawflag & 1) {
		for (i = 0; i < npts - 1; i++)
			s_line(pts[i * 2], pts[i * 2 + 1], pts[i * 2 + 2], pts[i * 2 + 3]);
		s_line(pts[i * 2], pts[i * 2 + 1], pts[0], pts[1]);
	}
}

static void
s_box(int x, int y, int width, int height, int drawflag)
{
	Int pts[10];
	if (width <= 0 || height <= 0)
		return;
	pts[0] = pts[6] = pts[8] = x;
	pts[1] = pts[3] = pts[9] = y;
	pts[2] = pts[4] = x + width;
	pts[5] = pts[7] = y + height;
	s_poly(pts, 5, drawflag);
}

static void
s_roundbox(int x, int y, int width, int height, int rx, int ry, int drawflag)
{
	Int pts[18];
	if (width <= 0 || height <= 0)
		return;
	if (rx > width / 2)
		rx = width / 2;
	if (ry > height / 2)
		ry = height / 2;
	if (rx <= 0 || ry <= 0)
		s_box(x, y, width, height, drawflag);

	pts[0] = pts[10] = pts[16] = x + rx;
	pts[2] = pts[8] = x + width - rx;
	pts[4] = pts[6] = x + width;
	pts[12] = pts[14] = x;
	pts[1] = pts[3] = pts[17] = y;
	pts[5] = pts[15] = y + ry;
	pts[7] = pts[13] = y + height - ry;
	pts[9] = pts[11] = y + height;
	
	if (drawflag & 2)
		s_polyfill(pts, 9);

	s_obarc(pts[0], pts[5], rx, ry, 180.0, 270.0, drawflag);
	s_obarc(pts[2], pts[5], rx, ry, 270.0, 0.0, drawflag);
	s_obarc(pts[2], pts[7], rx, ry, 0.0, 90.0, drawflag);
	s_obarc(pts[0], pts[7], rx, ry, 90.0, 180.0, drawflag);
	
	if (drawflag & 1) {
		s_line(pts[0], pts[1], pts[2], pts[3]);
		s_line(pts[4], pts[5], pts[6], pts[7]);
		s_line(pts[8], pts[9], pts[10], pts[11]);
		s_line(pts[12], pts[13], pts[14], pts[15]);
	}
}

#if 0
#pragma mark ====== BASIC Interface ======
#endif

/*  GCOLOR col
    Set the color for stroking lines  */
int
bs_builtin_gcolor(void)
{
	Int n1;
	if (bs_get_next_int_arg(&n1) == 0 || n1 == 0)
		s_strokeflag = 0;
	else {
		s_strokeflag = 1;
		s_gcolor = n1;
	}
	return 0;
}

/*  FILLCOLOR col
    Set the color for painting  */
int
bs_builtin_fillcolor(void)
{
	Int n1;
	if (bs_get_next_int_arg(&n1) == 0 || n1 == 0)
		s_fillflag = 0;
	else {
		s_fillflag = 2;
		s_fillcolor = n1;
	}
	return 0;
}

/*  PSET x, y
    Draw a dot  */
int
bs_builtin_pset(void)
{
	Int x, y;
	bs_get_next_int_arg(&x);
	bs_get_next_int_arg(&y);
	if (x >= 0 && x < my_width && y >= 0 && y < my_height) {
		bs_select_active_buffer(GRAPHIC_ACTIVE);
		bs_pset_nocheck(x, y, s_gcolor);
	}
	s_last_x = x;
	s_last_y = y;
	return 0;
}

/*  LINE x1, y1, x2, y2
    Draw a line.  */
int
bs_builtin_line(void)
{
	Int x1, y1, x2, y2;
	bs_get_next_int_arg(&x1);
	bs_get_next_int_arg(&y1);
	bs_get_next_int_arg(&x2);
	bs_get_next_int_arg(&y2);
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	s_line(x1, y1, x2, y2);
	s_last_x = x2;
	s_last_y = y2;
	return 0;
}

/*  LINETO x1, x2
    Draw a line from the last point.  */
int
bs_builtin_lineto(void)
{
	Int x1, y1;
	bs_get_next_int_arg(&x1);
	bs_get_next_int_arg(&y1);
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	s_line(s_last_x, s_last_y, x1, y1);
	s_last_x = x1;
	s_last_y = y1;
	return 0;
}

/*  CIRCLE ox, oy, rad1 [, rad2]
    Draw a circle. If rad2 is not given, then rad1 = rad2 is assumed.  */
int
bs_builtin_circle(void)
{
	Int ox, oy, rad1, rad2;
	bs_get_next_int_arg(&ox);
	bs_get_next_int_arg(&oy);
	bs_get_next_int_arg(&rad1);	
	if (bs_get_next_int_arg(&rad2) == 0)
		rad2 = rad1;
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	s_obarc(ox, oy, rad1, rad2, 0.0, 360.0, s_strokeflag | s_fillflag);
	s_last_x = ox;
	s_last_y = oy;
	return 0;
}

/*  ARC ox, oy, start#, end#, rad1 [, rad2]
    Draw a part of circle. Start and end are the start and end angles (in degree)  */
int
bs_builtin_arc(void)
{
	Int ox, oy, rad1, rad2;
	Float start, end;
	bs_get_next_int_arg(&ox);
	bs_get_next_int_arg(&oy);
	bs_get_next_float_arg(&start);
	bs_get_next_float_arg(&end);
	bs_get_next_int_arg(&rad1);	
	if (bs_get_next_int_arg(&rad2) == 0)
		rad2 = rad1;
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	s_obarc(ox, oy, rad1, rad2, start, end, s_strokeflag | s_fillflag);
	s_last_x = ox;
	s_last_y = oy;
	return 0;
}

/*  FAN ox, oy, start#, end#, rad1 [, rad2]
    Draw a fan. Start and end are the start and end angles (in degree)  */
int
bs_builtin_fan(void)
{
	Int ox, oy, rad1, rad2;
	Float start, end, d;
	Int p[8];
	bs_get_next_int_arg(&ox);
	bs_get_next_int_arg(&oy);
	bs_get_next_float_arg(&start);
	bs_get_next_float_arg(&end);
	bs_get_next_int_arg(&rad1);	
	if (bs_get_next_int_arg(&rad2) == 0)
		rad2 = rad1;
	if (start != 0.0 || end != 360.0 || s_fillflag) {
		d = end - start;
		d -= floor(d / 360.0) * 360.0;
		if (d > 180.0) {
			/*  Draw and fill two arcs separately  */
			d = start + 180.0;
			d -= floor(d / 360.0) * 360.0;
		} else d = start;
	}
	p[0] = ox;
	p[1] = oy;
	p[2] = ox + floor(rad1 * cos(d * (PI / 180.0)) + 0.5);
	p[3] = oy + floor(rad2 * sin(d * (PI / 180.0)) + 0.5);
	p[4] = ox + floor(rad1 * cos(end * (PI / 180.0)) + 0.5);
	p[5] = oy + floor(rad2 * sin(end * (PI / 180.0)) + 0.5);
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	if (s_fillflag) {
		s_polyfill(p, 3);
	}
	if (d != start)
		s_obarc(ox, oy, rad1, rad2, start, d, s_strokeflag | s_fillflag);
	s_obarc(ox, oy, rad1, rad2, d, end, s_strokeflag | s_fillflag);		
	if (s_strokeflag) {
		s_line(ox, oy, ox + floor(rad1 * cos(start * (PI / 180.0)) + 0.5), oy + floor(rad2 * sin(start * (PI / 180.0)) + 0.5));
		s_line(ox, oy, p[4], p[5]);
	}
	s_last_x = ox;
	s_last_y = oy;
	return 0;	
}

/*  POLY x1, y1, ..., xn, yn
    Draw a polygon.  */
int
bs_builtin_poly(void)
{
	Int *buf;
	int i, n;
	void *p;  /*  Dummy  */
	n = 0;
	while (bs_get_next_arg_type_and_ptr(&p) != 0)
		n++;
	if (n % 2 != 0) {
		bs_runtime_error("The y-coordinates of the last point is missing");
		return 1;
	}
	buf = (Int *)alloca(sizeof(Int) * n);
	if (buf == NULL) {
		bs_runtime_error("Cannot allocate buffer for drawing polygon");
		return 1;
	}
	bs_reset_arg_accessor();
	for (i = 0; i < n; i++) {
		if (bs_get_next_int_arg(buf + i) == 0) {
			bs_runtime_error("The coordinates must be numbers");
			return 1;
		}
	}
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	s_poly(buf, n / 2, s_strokeflag | s_fillflag);
	s_last_x = buf[n - 2];
	s_last_y = buf[n - 1];	
	return 0;
}

/*  BOX x1, y1, width, height
    Draw a box.  */
int
bs_builtin_box(void)
{
	Int x1, y1, width, height;
	bs_get_next_int_arg(&x1);
	bs_get_next_int_arg(&y1);
	bs_get_next_int_arg(&width);
	bs_get_next_int_arg(&height);
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	s_box(x1, y1, width, height, s_strokeflag | s_fillflag);
	s_last_x = x1;
	s_last_y = y1;
	return 0;
}

/*  RBOX x1, y1, width, height, rx[, ry]
    Draw a round-corner box.  */
int
bs_builtin_rbox(void)
{
	Int x1, y1, width, height, rx, ry;
	bs_get_next_int_arg(&x1);
	bs_get_next_int_arg(&y1);
	bs_get_next_int_arg(&width);
	bs_get_next_int_arg(&height);
	bs_get_next_int_arg(&rx);
	if (bs_get_next_int_arg(&ry) == 0)
		ry = rx;
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	s_roundbox(x1, y1, width, height, rx, ry, s_strokeflag | s_fillflag);
	s_last_x = x1;
	s_last_y = y1;
	return 0;
}

/*  GCLS
    Clear the graphic screen  */
int
bs_builtin_gcls(void)
{
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	bs_clear_box(0, 0, my_width, my_height);
	return 0;
}

/*  GMODE g
    Change the graphic mode. g=0: normal, g=1: 2x2 expanded screen  */
int
bs_builtin_gmode(void)
{
	Int mode;
	bs_get_next_int_arg(&mode);
	bs_gmode_platform(mode);
	my_graphic_mode = mode;
	my_max_x = my_width / 8;
	my_max_y = my_height / 16;
	bs_builtin_cls();
	bs_builtin_gcls();
	return 0;
}

/*  PATDEF num, width, height, table
    Define a pattern num (num in 1..256).
    Table is an array of color codes (column first).  */
int
bs_builtin_patdef(void)
{
	Int width, height, len, i, num, len2, *ip, size;
	PatInfo *pip;
	Off_t dimref, patinfo_top;
	pixel_t *p;
	bs_get_next_int_arg(&num);
	bs_get_next_int_arg(&width);
	bs_get_next_int_arg(&height);
	bs_get_next_str_arg(&dimref);
	dimref &= ~kMSBOff;
	
	if (num < 1 || num > 256) {
		bs_runtime_error("The pattern number must be in 1..256");
		return 1;
	}
	if (width <= 0 || height <= 0) {
		bs_runtime_error("The width and height must be positive numbers");
		return 1;
	}
	len = width * height;
	num--;
	
	patinfo_top = (num + 1) * sizeof(PatInfo);
	if (patinfo_top >= gPatInfoEnd - gPatInfoBase) {
		size = (sizeof(PatInfo) * (num + 2) / 1024 + 1) * 1024;
		if (bs_realloc_block(MEM_PATINFO, size) < 0) {
			bs_runtime_error("Out of memory: too many patterns");
			return 1;
		}
	}
	if (gPatInfoTop < patinfo_top)
		gPatInfoTop = patinfo_top;

	pip = (PatInfo *)gPatInfoBasePtr + num;
	len2 = pip->width * pip->height;
	
	if (gPatternTop + (len - len2) * sizeof(pixel_t) >= gPatternEnd - gPatternBase) {
		size = ((gPatternTop + (len - len2) * sizeof(pixel_t)) / 1024 + 1) * 1024;
		if (bs_realloc_block(MEM_PATTERN, size) < 0) {
			bs_runtime_error("Out of memory: pattern is too large");
			return 1;
		}
	}
	
	if (len2 == 0) {
		/*  New entry  */
		if (num == 0)
			pip->ofs = 0;
		else
			pip->ofs = (pip - 1)->ofs + (pip - 1)->width * (pip - 1)->height;
	}		
	/*  Move the already existing data  */
	p = (pixel_t *)gPatternBasePtr + pip->ofs;
	memmove(p + len, p + len2, gPatternTop - sizeof(pixel_t) * (pip->ofs + len2));
	gPatternTop += (len - len2) * sizeof(pixel_t);
	
	/*  Update offsets for patterns[i] (i > num)  */
	for (i = num + 1; i < gPatInfoTop / sizeof(PatInfo); i++)
		((PatInfo *)gPatInfoBasePtr)[i].ofs += len - len2;
	pip->width = width;
	pip->height = height;

	/*  Read integer data from the DIM  */
	i = *((Off_t *)(gVarBasePtr + dimref));  /*  Dimension  */
	dimref += sizeof(Off_t) * (i + 1);
	len2 = 1;
	while (i > 0) {
		len2 *= ((Off_t *)(gVarBasePtr + dimref))[-i];
		i--;
	}
	if (len2 < len) {
		bs_runtime_error("Pattern data too short");
		return 1;
	}
	ip = (Int *)(gVarBasePtr + dimref);
	for (i = 0; i < len; i++)
		*p++ = *ip++;

	return 0;
}

/*  PATUNDEF num
    Forget the pre-defined pattern num.  */
int
bs_builtin_patundef(void)
{
	Int i, len2, num;
	pixel_t *p;
	PatInfo *pip;
	bs_get_next_int_arg(&num);
	if (num < 1 || num > 256) {
		bs_runtime_error("The pattern number must be in 1..256");
		return 1;
	}	
	num--;
	pip = (PatInfo *)gPatInfoBasePtr + num;

	/*  Move the already existing data  */
	p = (pixel_t *)gPatternBasePtr + pip->ofs;
	len2 = pip->width * pip->height;
	memmove(p, p + len2, gPatternTop - sizeof(pixel_t) * (pip->ofs + len2));
	
	/*  Update offsets for patterns[i] (i > num)  */
	for (i = num + 1; i < gPatInfoTop / sizeof(PatInfo); i++)
		((PatInfo *)gPatInfoBasePtr)[i].ofs -= len2;
	pip->width = 0;
	pip->height = 0;
	return 0;
}

static int
s_bs_builtin_patdraw_sub(Int n, Int x1, Int y1, pixel_t mask, int eraseflag)
{
	Int size;
	pixel_t *p1, *p2;
	PatInfo *pp;
	if (n <= 0 || n > gPatInfoTop / sizeof(PatInfo)) {
		bs_runtime_error("pattern number (%d) is out of range", n);
		return 1;
	}
	n--;
	pp = (PatInfo *)gPatInfoBasePtr + n;
	if (pp->width == 0) {
		bs_runtime_error("pattern %d is undefined", n + 1);
		return 1;
	}

	/*  Use pattern memory for temporary image processing  */
	size = sizeof(pixel_t) * pp->width * pp->height;
	if (gPatternTop + size >= gPatternEnd - gPatternBase) {
		int asize = gPatternEnd - gPatternBase + (size / 4096 + 1) * 4096;
		if (bs_realloc_block(MEM_PATTERN, asize) < 0) {
			bs_runtime_error("cannot allocate memory during pattern draw");
			return 1;
		}
	}
	p1 = (pixel_t *)(gPatternBasePtr + gPatternTop);
	bs_get_pattern(p1, x1, y1, pp->width, pp->height);
	p2 = (pixel_t *)(gPatternBasePtr + pp->ofs);
	for (n = pp->width * pp->height - 1; n >= 0; n--) {
		pixel_t col = *p2;
		if (eraseflag == 0) {
			if (col != 0 && mask != (BYTES_PER_PIXEL == 2 ? 0xffff : 0xffffffff)) {
				pixel_t r, g, b, a;
				r = REDCOMPINT(col) * REDCOMPINT(mask) / (REDCOMPINTMAX - 1);
				g = GREENCOMPINT(col) * GREENCOMPINT(mask) / (GREENCOMPINTMAX - 1);
				b = BLUECOMPINT(col) * BLUECOMPINT(mask) / (BLUECOMPINTMAX - 1);
				a = ALPHACOMPINT(col);
				col = RGBAINT(r, g, b, a);
			}
			*p1 = col;
		} else {
			if (col != 0)
				*p1 = 0;
		}
		p1++;
		p2++;
	}
	p1 = (pixel_t *)(gPatternBasePtr + gPatternTop);
	bs_select_active_buffer(GRAPHIC_ACTIVE);
	bs_put_pattern(p1, x1, y1, pp->width, pp->height);
	return 0;
}

/*  PATDRAW n, x, y [, mask]
 Draw the defined pattern (n) at position (x, y).
 Mask is a color mask, where the original RGB components are attenuated by this RGB values. */
int
bs_builtin_patdraw(void)
{
	Int n, x1, y1, mask;
	bs_get_next_int_arg(&n);
	bs_get_next_int_arg(&x1);
	bs_get_next_int_arg(&y1);
	if (bs_get_next_int_arg(&mask) == 0)
		mask = (BYTES_PER_PIXEL == 2 ? 0xffff : 0xffffffff);
	return s_bs_builtin_patdraw_sub(n, x1, y1, mask, 0);
}

/*  PATERASE n, x, y
 Erase the defined pattern (n) at position (x, y).  */
int
bs_builtin_paterase(void)
{
	Int n, x1, y1;
	bs_get_next_int_arg(&n);
	bs_get_next_int_arg(&x1);
	bs_get_next_int_arg(&y1);
	return s_bs_builtin_patdraw_sub(n, x1, y1, 0, 1);
}

/*  RGB(r#, g#, b#[, a#])
 Return a color code. The arguments are floating number (0.0 to 1.0).  */
int
bs_builtin_rgb(void)
{
	Float r, g, b, a;
	bs_get_next_float_arg(&r);
	bs_get_next_float_arg(&g);
	bs_get_next_float_arg(&b);
	if (bs_get_next_float_arg(&a) == 0)
		a = 1.0;
	bs_push_int_value(RGBAFLOAT(r, g, b, a));
	return 0;
}

/*  REDRAW(n)
 Control the screen redraw. n=0, Stop automatic redraw; n>0, Restart automatic redraw;
 n<0, Redraw immediately and stop further automatic redraw  */
int
bs_builtin_redraw(void)
{
	Int n;
	bs_get_next_int_arg(&n);
	if (n == 0)
		my_suppress_update = 1;
	else if (n > 0)
		my_suppress_update = 0;
	else {
		my_suppress_update = 0;
		bs_update_screen();
		my_suppress_update = 1;
	}
	return 0;
}
