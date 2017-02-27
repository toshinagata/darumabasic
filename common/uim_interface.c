/*
 *  uim_interface.c
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

#include "uim/uim.h"

int
init_im(void)
{
	if (uim_init() != 0) {
		printf("uim_init() failed\n");
		return 1;
	}
	return 0;
}
