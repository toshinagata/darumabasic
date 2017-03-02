//
//  MyAppDelegate.m
//  Daruma Basic
//
//  Created by Toshi Nagata on 16/01/09.
//  Copyright 2016 Toshi Nagata. All rights reserved.
//

#import "MyAppDelegate.h"
#include "daruma.h"
#include "screen.h"

@implementation MyAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	/*  Set current directory to the same directory as .app  */
	NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
	chdir([[bundlePath stringByDeletingLastPathComponent] fileSystemRepresentation]);
	[window makeKeyAndOrderFront:self];
	my_graphic_mode = 1;
	bs_runloop();
	[NSApp terminate:self];
}

@end
