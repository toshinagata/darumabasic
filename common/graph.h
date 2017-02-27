/*
 *  graph.h
 *  Daruma Basic
 *
 *  Created by Toshi Nagata on 16/01/09.
 *  Copyright 2016 Toshi Nagata. All rights reserved.
 *
 */

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "daruma.h"

extern int bs_builtin_gcolor(void);
extern int bs_builtin_fillcolor(void);
extern int bs_builtin_pset(void);
extern int bs_builtin_line(void);
extern int bs_builtin_lineto(void);
extern int bs_builtin_circle(void);
extern int bs_builtin_arc(void);
extern int bs_builtin_fan(void);
extern int bs_builtin_poly(void);
extern int bs_builtin_box(void);
extern int bs_builtin_rbox(void);
extern int bs_builtin_gcls(void);
extern int bs_builtin_gmode(void);
extern int bs_builtin_patdef(void);
extern int bs_builtin_patundef(void);
extern int bs_builtin_patdraw(void);
extern int bs_builtin_paterase(void);
extern int bs_builtin_rgb(void);
extern int bs_builtin_redraw(void);

#endif /* __GRAPH_H__ */
